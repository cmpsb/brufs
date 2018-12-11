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

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include <stdexcept>

#include "FdAbst.hpp"
#include "BrufsOpener.hpp"

namespace Brufscli {

class BadBrufsException : public std::runtime_error {
    using std::runtime_error::runtime_error;
};

}

Brufscli::BrufsInstance Brufscli::BrufsOpener::open(const std::string &path) const {
    int iofd = ::open(path.c_str(), O_RDWR);
    if (iofd == -1) {
        throw std::runtime_error("Unable to open " + path + ": " + strerror(errno));
    }

    auto io = new FdAbst(iofd);
    auto disk = new Brufs::Disk(io);
    auto fs = new Brufs::Brufs(disk);

    return BrufsInstance(
        std::shared_ptr<Brufs::Brufs>(fs),
        std::shared_ptr<Brufs::Disk>(disk),
        std::shared_ptr<FdAbst>(io)
    );
}

Brufscli::BrufsInstance Brufscli::BrufsOpener::open_new(const std::string &path) const {
    return this->open(path);
}

Brufscli::BrufsInstance Brufscli::BrufsOpener::open_existing(const std::string &path) const {
    auto instance = this->open(path);

    auto &io = instance.get_io();
    auto &fs = instance.get_fs();
    auto status = fs.get_status();
    if (status < Brufs::Status::OK) {
        throw BadBrufsException(
            "Unable to load a filesystem from " + path + ": " + io.strstatus(status)
        );
    }

    return instance;
}
