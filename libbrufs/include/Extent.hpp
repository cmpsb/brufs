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

#include "types.hpp"

namespace Brufs {

/**
 * An extent describing the start of a contiguous bunch of data and how long it is
 */
struct Extent {
    /**
     * The start LBA of the extent
     */
    Address offset;

    /**
     * The length of the extent in blocks
     */
    Size length;

    Extent() {}

    Extent(const Extent &other) = default;
    template <typename T>
    Extent(const T &other) : offset(other.offset), length(other.length) {}
    Extent(Address offset, Size length) : offset(offset), length(length) {}

    auto operator==(const Extent &other) const {
        return this->offset == other.offset && this->length == other.length;
    }

    Offset get_end() {
        return this->offset + length;
    }

    Offset get_last() {
        return this->get_end() - 1;
    }
};
static_assert(
    std::is_standard_layout<Extent>::value, "the extent structure must be standard-layout"
);

}
