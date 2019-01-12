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
#include <cstring>

#include <random>

#include "libbrufs.hpp"
#include "Util.hpp"

static constexpr unsigned int PPT_BUF_SIZE = 32;

Brufs::PrettyPrint Brufuse::Util::pretty_print;

Brufs::String Brufuse::Util::pretty_print_bytes(__uint128_t bytes) {
    return pretty_print.pp_size(bytes);
}

Brufs::String Brufuse::Util::pretty_print_inode_id(Brufs::InodeId inode_id) {
    return pretty_print.pp_inode_id(inode_id);
}

Brufs::String Brufuse::Util::pretty_print_mode(bool is_dir, uint16_t mode) {
    return pretty_print.pp_mode(is_dir, mode);
}

Brufs::String Brufuse::Util::pretty_print_timestamp(const Brufs::Timestamp &ts) {
    char buf[PPT_BUF_SIZE];

    struct tm tm;
    localtime_r((const time_t *) &ts.seconds, &tm);
    strftime(buf, PPT_BUF_SIZE, "%F %T", &tm);

    return {buf};
}

Brufs::InodeId Brufuse::Util::generate_inode_id(char alt) {
    std::random_device rd;
    std::mt19937 mt(rd());
    std::uniform_int_distribution<uint64_t> dist;

    return (static_cast<Brufs::InodeId>(dist(mt)) << 6) | alt;
}
