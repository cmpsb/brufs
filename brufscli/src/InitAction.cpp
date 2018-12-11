/*
 * Copyright (c) 2017-2018 Luc Everse <luc@wukl.net>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#include <cmath>

#include "InitAction.hpp"

std::vector<std::string> Brufscli::InitAction::get_names() const {
    return {"init"};
}

std::vector<slopt_Option> Brufscli::InitAction::get_options() const {
    return {
        {'c', "cluster-size", SLOPT_REQUIRE_ARGUMENT},
        {'l', "sc-low-mark", SLOPT_REQUIRE_ARGUMENT},
        {'h', "sc-high-mark", SLOPT_REQUIRE_ARGUMENT}
    };
}

void Brufscli::InitAction::apply_option(
    int sw, [[maybe_unused]] int snam, [[maybe_unused]] const std::string &lnam, 
    const std::string &val
) {
    if (sw == SLOPT_DIRECT && this->dev_path.empty()) {
        this->dev_path = val;
        return;
    }

    if (sw == SLOPT_DIRECT) {
        throw InvalidArgumentException(
            "Don't know what to do with \"" + val + "\" (device path is " + this->dev_path + ")"
        );
    }

    switch (snam) {
    case 'c': {
        auto lval = std::stoul(val);
        if (lval < 32) {
            this->cluster_size_exp = static_cast<uint32_t>(lval);
        } else {
            this->cluster_size_exp = static_cast<uint32_t>(std::log2(lval));
            if ((1UL << this->cluster_size_exp) != lval) {
                throw InvalidArgumentException("Cluster size must be a power of two");
            }
        }
        break;
    }

    case 'l': {
        auto lval = std::stoul(val);

        this->sc_low_mark = static_cast<uint8_t>(lval);
        if (this->sc_low_mark != lval) {
            throw InvalidArgumentException("Spare cluster low mark is too high; maximum is 255");
        }

        break;
    }

    case 'h': {
        auto lval = std::stoul(val);

        this->sc_high_mark = static_cast<uint8_t>(lval);
        if (this->sc_high_mark != lval) {
            throw InvalidArgumentException("Spare cluster high mark is too high; maximum is 255");
        }

        break;
    }
    }
}

void Brufscli::InitAction::validate_cluster_size() {
    if (this->cluster_size_exp < 9) {
        throw InvalidArgumentException("Cluster size must be at least 512 bytes");
    } else if (this->cluster_size_exp < 12) {
        this->logger.note(
            "Cluster sizes smaller than 4096 bytes have a significant performance impact"
        );
    } else if (this->cluster_size_exp > 20) {
        this->logger.warn("Very large cluster sizes waste enormous amounts of space");
    } else if (this->cluster_size_exp > 16) {
        this->logger.note("Cluster sizes larger than 65536 may waste space");
    }
}

void Brufscli::InitAction::validate_spare_marks() {
    auto suggested_low = this->suggest_sc_low_mark();

    if (this->sc_low_mark < suggested_low) {
        this->logger.warn(
            "A very low spare cluster count could cause intermittent allocation failures"
        );
        this->logger.warn("Suggested low mark: %hhu", suggested_low);
    }

    if (this->sc_high_mark < 1.5 * this->sc_low_mark) {
        this->logger.note(
            "The high mark should usually be at least 150%% of the low mark to prevent "
            "unexpected allocations during irrelevant operations"
        );
    }
}

uint8_t Brufscli::InitAction::suggest_sc_low_mark() {
    return static_cast<uint8_t>(std::ceil(150. / this->cluster_size_exp));
}

int Brufscli::InitAction::run([[maybe_unused]] const std::string &name) {
    if (this->sc_low_mark == 0) this->sc_low_mark = this->suggest_sc_low_mark();
    if (this->sc_high_mark == 0) this->sc_high_mark = 2 * this->sc_low_mark;

    auto brufs = this->opener.open_new(this->dev_path);
    auto &fs = brufs.get_fs();
    auto &io = brufs.get_io();

    Brufs::Header proto;
    proto.cluster_size_exp = this->cluster_size_exp;
    proto.sc_low_mark = this->sc_low_mark;
    proto.sc_high_mark = this->sc_high_mark;

    auto status = fs.init(proto);
    this->on_error(status, "Unable to initialize the filesystem: ", io);

    return status;
}
