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

#pragma once

#include <cstring>

#include <vector>

#include "AbstIO.hpp"
#include "Status.hpp"

class MemIO : public Brufs::AbstIO {
private:
    std::vector<char> mbuf;

public:
    MemIO(size_t size) : mbuf(size) {}

    Brufs::SSize read(void *buf, Brufs::Size count, Brufs::Address offset) const override {
        if (count + offset > this->mbuf.size()) return Brufs::Status::E_DISK_TRUNCATED;
        auto actual_count = std::min<size_t>(count + offset, this->mbuf.size()) - offset;

        memcpy(static_cast<char *>(buf), this->mbuf.data() + offset, actual_count);

        return actual_count;
    }

    Brufs::SSize write(const void *buf, Brufs::Size count, Brufs::Address offset) override {
        if (count + offset > this->mbuf.size()) return Brufs::Status::E_DISK_TRUNCATED;
        auto actual_count = std::min<size_t>(count + offset, this->mbuf.size()) - offset;

        memcpy(this->mbuf.data() + offset, static_cast<const char *>(buf), actual_count);

        return actual_count;
    }

    const char *strstatus(Brufs::SSize eno) const override {
        return Brufs::strerror(static_cast<Brufs::Status>(eno));
    }

    Brufs::Size get_size() const override {
        return this->mbuf.size();
    }

    void resize(size_t new_size) {
        this->mbuf.resize(new_size);
    }
};
