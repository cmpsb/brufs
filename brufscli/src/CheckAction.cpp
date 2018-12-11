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

#include "CheckAction.hpp"
#include "Util.hpp"

std::vector<std::string> Brufscli::CheckAction::get_names() const {
    return {"check"};
}

void Brufscli::CheckAction::apply_option(
    int sw,
    [[maybe_unused]] int snam, [[maybe_unused]] const std::string &lnam,
    const std::string &val
) {
    if (sw == SLOPT_DIRECT && this->spec.empty()) {
        this->spec = val;
        return;
    }

    if (sw == SLOPT_DIRECT) {
        throw InvalidArgumentException(
            "Unexpected value " + val + " (path is " + this->spec + ")"
        );
    }
}

int Brufscli::CheckAction::run([[maybe_unused]] const std::string &name) {
    auto path = this->path_parser.parse({this->spec.c_str(), this->spec.length()});
    this->path_validator.validate(path, true, false);

    auto brufs = this->opener.open_existing(path.get_partition());
    auto &fs = brufs.get_fs();
    const auto &io = brufs.get_io();

    const auto capacity = fs.get_header().num_blocks;
    Brufs::Size reserved, available, extents, in_fbt;
    auto status = fs.count_free_blocks(reserved, available, extents, in_fbt);
    this->on_error(status, "Unable to query global space usage: ", io);

    int available_pct = (available * 100) / capacity;
    int reserved_pct = (reserved * 100) / capacity;

    auto cap_str = Util::pretty_print_bytes(capacity);
    auto avail_str = Util::pretty_print_bytes(available);
    auto res_str = Util::pretty_print_bytes(reserved);

    this->logger.info("Capacity: %s (%lu)", cap_str.c_str(), capacity);
    this->logger.info("Available: %s (%lu) in %lu extents (%d%%)",
        avail_str.c_str(), available, extents, available_pct
    );
    this->logger.info("Reserved: %s (%lu, %d%%)", res_str.c_str(), reserved, reserved_pct);

    int in_fbt_pct = (in_fbt * 100) / capacity;
    auto in_fbt_str = Util::pretty_print_bytes(in_fbt);

    this->logger.info("Free space tree at: 0x%lX, %s (%d%%)",
        fs.get_header().fbt_address, in_fbt_str.c_str(), in_fbt_pct
    );

    Brufs::SSize root_count = fs.count_roots();
    this->logger.info("%lld roots", root_count);

    std::vector<Brufs::RootHeader> roots(root_count);
    status = static_cast<Brufs::Status>(fs.collect_roots(roots.data(), root_count));
    this->on_error(status, "Can't read roots: ", io);

    for (Brufs::SSize i = 0; i < root_count; ++i) {
        Brufs::Root root(fs, roots[i]);

        this->logger.info("Root \"%s\"", roots[i].label);
        this->logger.info("  int at 0x%lX", root.get_header().int_address);
        this->logger.info("  ait at 0x%lX", root.get_header().ait_address);
    }

    this->logger.info("OK\n");

    return 0;
}
