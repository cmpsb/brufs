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

#include <cstdio>
#include <cerrno>
#include <cstring>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include <stdexcept>

#include <uv.h>

#include "libbrufs.hpp"

#include "service.hpp"
#include "FdAbst.hpp"

Brufuse::FdAbst *Brufuse::fs_io;
Brufs::Brufs *Brufuse::fs;

uv_rwlock_t Brufuse::fs_rwlock;

namespace Brufuse {

class BrufsException : public std::runtime_error {
    using std::runtime_error::runtime_error;
};

}

void Brufuse::open_fs(const std::string &dev_path) {
    int iofd = open(dev_path.c_str(), O_RDWR);
    if (iofd == -1) {
        throw BrufsException("Unable to open " + dev_path + ": " + strerror(errno));
    }

    fs_io = new FdAbst(iofd);
    fs = new Brufs::Brufs(new Brufs::Disk(fs_io));
    if (fs->get_status() < Brufs::Status::OK) {
        throw BrufsException(
            "Unable to open " + dev_path + ": " + fs_io->strstatus(fs->get_status())
        );
    }

    uv_rwlock_init(&fs_rwlock);
}
