/*b+tree
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

#include <cstdio>
#include <cassert>
#include <cerrno>
#include <linux/fs.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#undef BLOCK_SIZE

#include "FdAbst.hpp"

FdAbst::FdAbst(int file) : file(file) {
    assert(file > 0);
}

Brufs::SSize FdAbst::read(void *buf, Brufs::Size count, Brufs::Address offset) const {
    ssize_t status = pread(this->file, buf, count, offset);
    if (status == -1) return Brufs::Status::E_ABSTIO_BASE + errno;

    return status;
}

Brufs::SSize FdAbst::write(const void *buf, Brufs::Size count, Brufs::Address offset) {
    ssize_t status = pwrite(this->file, buf, count, offset);
    if (status == -1) return Brufs::Status::E_ABSTIO_BASE + errno;

    return status;
}

const char *FdAbst::strstatus(Brufs::SSize eno) const {
    if (eno < Brufs::E_ABSTIO_BASE || eno >= Brufs::Status::OK) {
        return Brufs::strerror(static_cast<Brufs::Status>(eno));
    }

    return strerror(eno - Brufs::Status::E_ABSTIO_BASE);
}

Brufs::Size FdAbst::get_size() const {
    struct stat st;
    fstat(this->file, &st);

    if (S_ISBLK(st.st_mode)) {
        uint64_t size;
        int status = ioctl(this->file, BLKGETSIZE64, &size);
        if (status == -1) return 0;

        return static_cast<Brufs::Size>(size);
    }

    return static_cast<Brufs::Size>(st.st_size);
}
