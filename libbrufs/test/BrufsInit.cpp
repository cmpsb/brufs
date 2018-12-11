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

#include "catch.hpp"

#include "MemIO.hpp"
#include "Brufs.hpp"

static constexpr size_t NORMAL_DISK_SIZE = 32 * 1024 * 1024;

TEST_CASE("Brufs initialization", "[lib]") {
    MemIO mem_io(NORMAL_DISK_SIZE);
    Brufs::Disk disk(&mem_io);
    Brufs::Brufs fs(&disk);

    Brufs::Header proto;
    proto.cluster_size_exp = 12;
    proto.sc_low_mark = 12;
    proto.sc_high_mark = 24;

    SECTION("Normal init is successful") {
        auto status = fs.init(proto);
        CHECK(status == Brufs::Status::OK);
    }

    SECTION("Tiny init fails with very small disk") {
        mem_io.resize(10 * 4024);
        auto status = fs.init(proto);
        CHECK(status != Brufs::Status::OK);
    }

    SECTION("Init fails with a very low spare cluster watermark") {
        proto.sc_low_mark = 0;
        proto.sc_high_mark = 0;

        auto status = fs.init(proto);
        CHECK(status != Brufs::Status::OK);
    }
}
