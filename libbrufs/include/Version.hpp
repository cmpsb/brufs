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

#include <type_traits>

#include <cstdint>

#include "types.hpp"

namespace Brufs {

struct Version {
    uint8_t major;
    uint8_t minor;
    uint16_t patch;

    static Version get();

    int compare(const Version &other) const;

    auto operator<(const Version &other) const { return this->compare(other) < 0; }
    auto operator>(const Version &other) const { return this->compare(other) > 0; }

    auto operator==(const Version &other) const { return this->compare(other) == 0; }
    auto operator!=(const Version &other) const { return this->compare(other) != 0; }

    auto operator<=(const Version &other) const { return *this == other || *this < other; }
    auto operator>=(const Version &other) const { return *this == other || *this > other; }

    int to_string(char *buf, Size len) const;
};
static_assert(
    std::is_standard_layout<Version>::value, "the version structure must be standard-layout"
);
static_assert(sizeof(Version) == 4, "the version structure must be exactly 4 bytes long");

}
