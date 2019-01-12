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

#include "PrettyPrint.hpp"

TEST_CASE("Pretty-printing", "[util]") {
    const Brufs::PrettyPrint pp;

    SECTION("Can pretty-print 0 bytes") {
        auto str = pp.pp_size(0);
        CHECK(str == "0.0 B");
    }

    SECTION("Can pretty-print a small number of bytes") {
        auto str = pp.pp_size(888);
        CHECK(str == "888.0 B");
    }

    SECTION("Can print a number of kiB") {
        auto str = pp.pp_size(1024);
        CHECK(str == "1.0 kiB");
    }

    SECTION("Can print a number of MiB") {
        auto str = pp.pp_size(14616742);
        CHECK(str == "13.9 MiB");
    }

    SECTION("Can pretty-print inode ID 0") {
        auto str = pp.pp_inode_id(0);
        CHECK(str == "0000:0000:0000:0000:0000:0000:0000:0000");
    }

    SECTION("Can pretty-print inode ID 396933") {
        auto str = pp.pp_inode_id(396933);
        CHECK(str == "0000:0000:0000:0000:0000:0000:0006:0E85");
    }

    SECTION("Can pretty-print inode ID 140640107354063169360504823800796502055") {
        Brufs::InodeId id = 0x69CE4CA65D018B70UL;
        id <<= 64;
        id |= 0x46F9EC3203E35827UL;
        auto str = pp.pp_inode_id(id);
        CHECK(str == "69CE:4CA6:5D01:8B70:46F9:EC32:03E3:5827");
    }

    SECTION("Can pretty-print an 0-mode file") {
        auto str = pp.pp_mode(false, 0);
        CHECK(str == "----------");
    }

    SECTION("Can pretty-print a 644 file") {
        auto str = pp.pp_mode(false, 0644);
        CHECK(str == "-rw-r--r--");
    }

    SECTION("Can pretty-print a 777 directory") {
        auto str = pp.pp_mode(true, 0777);
        CHECK(str == "drwxrwxrwx");
    }
}
