/*
 * Copyright (c) 2017-2018 Luc Everse <luc@cmpsb.net>
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

#include <cstdio>
#include <cassert>
#include <cerrno>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#include "fd_abst.hpp"

fd_abst::fd_abst(int file) : file(file) {
    assert(file > 0);
}

brufs::ssize fd_abst::read(void *buf, brufs::size count, brufs::address offset) const {
    ssize_t status = pread(this->file, buf, count, offset);
    if (status == -1) return brufs::status::E_ABSTIO_BASE + errno;

    return status;
}

brufs::ssize fd_abst::write(const void *buf, brufs::size count, brufs::address offset) {
    ssize_t status = pwrite(this->file, buf, count, offset);
    if (status == -1) return brufs::status::E_ABSTIO_BASE + errno;

    return status;
}

const char *fd_abst::strstatus(brufs::ssize eno) const {
    return strerror(eno - brufs::status::E_ABSTIO_BASE);
}

brufs::size fd_abst::get_size() const {
    struct stat st;
    fstat(this->file, &st);

    return static_cast<brufs::size>(st.st_size);
}
