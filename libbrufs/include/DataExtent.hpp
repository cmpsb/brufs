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
#include "Extent.hpp"

namespace Brufs {

/**
 * An extent describing data in a file.
 *
 * Used in small to big files where large amounts of zeroes are represented by holes, or missing
 * extents. This extent includes a file offset indicating where the actual data begins, instead
 * of following the previous extent.
 */
struct DataExtent {
    /**
     * The start LBA of the extent
     */
    Address offset;

    /**
     * The length of the extent in blocks
     */
    Size length;

    /**
     * The start of the data in the file.
     */
    Offset local_start;

    DataExtent() = default;
    DataExtent(const DataExtent &other) = default;
    DataExtent(const Extent &other, const Offset local_start) :
        offset(other.offset),
        length(other.length),
        local_start(local_start)
    {}

    Offset get_local_end() const {
        return this->local_start + this->length;
    }

    Offset get_local_last() const {
        return this->get_local_end() - 1;
    }

    bool contains_local(const Offset offset) const {
        return offset >= this->local_start && offset < this->get_local_end();
    }

    Offset relativize_local(const Offset offset) const {
        return offset - this->local_start;
    }
};
static_assert(
    std::is_standard_layout<DataExtent>::value,
    "the data extent structure must be standard-layout"
);



}
