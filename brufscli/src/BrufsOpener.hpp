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

#include <memory>
#include <string>

#include "libbrufs.hpp"

namespace Brufscli {

class BrufsInstance {
private:
    std::shared_ptr<Brufs::Brufs> fs;
    std::shared_ptr<Brufs::Disk> disk;
    std::shared_ptr<Brufs::AbstIO> io;

public:
    BrufsInstance(
        std::shared_ptr<Brufs::Brufs> fs,
        std::shared_ptr<Brufs::Disk> disk,
        std::shared_ptr<Brufs::AbstIO> io
    ) :
        fs(fs), disk(disk), io(io)
    {}


    Brufs::AbstIO &get_io() {
        return *this->io;
    }

    const Brufs::AbstIO &get_io() const {
        return *this->io;
    }

    Brufs::Disk &get_disk() {
        return *this->disk;
    }

    const Brufs::Disk &get_disk() const {
        return *this->disk;
    }

    Brufs::Brufs &get_fs() {
        return *this->fs;
    }

    const Brufs::Brufs &get_fs() const {
        return *this->fs;
    }
};

class BrufsOpener {
private:
    BrufsInstance open(const std::string &path) const;

public:
    virtual BrufsInstance open_new(const std::string &path) const;
    virtual BrufsInstance open_existing(const std::string &path) const;

    virtual BrufsInstance open_new(const Brufs::String &path) const {
        return this->open_new(std::string(path.c_str(), path.length()));
    }

    virtual BrufsInstance open_existing(const Brufs::String &path) const {
        return this->open_existing(std::string(path.c_str(), path.length()));
    }
};

}
