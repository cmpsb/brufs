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
#include <cassert>

#include "config.hpp"
#include "Version.hpp"

int Brufs::Version::compare(const Version &other) const {
    if (this->major == 0 && other.minor == 0 && other.major == 0 && other.minor == 0) {
        return (this->patch == other.patch) ? 0 : -1000;
    }

    if (other.major == 0 && other.minor == 0) return -1000;

    if (this->major > other.major) return 100;
    if (this->major < other.major) return -100;

    if (this->minor > other.minor) return 10;
    if (this->minor < other.minor) return -10;

    // Ignore patch differences
    return 0;
}

int Brufs::Version::to_string(char *buf, size_t len) const {
    return snprintf(buf, len, "%hhu.%hhu.%hu", this->major, this->minor, this->patch);
}

/**
 * Fills the given version structure with the library's version information.
 *
 * @param version the version struct to fill
 */
Brufs::Version Brufs::Version::get() {
    return {
        BRUFS_VERSION_MAJOR,
        BRUFS_VERSION_MINOR,
        BRUFS_VERSION_PATCH
    };
}
