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

#include <cassert>

#include "internal.hpp"

const brufs::address brufs::extent::END = 0;

brufs::status brufs::brufs::allocate_blocks(size length, extent &target) {
    extent result;
    status status = this->fbt.remove(length, result);
    if (status < 0) return status;

    if (result.length > length) {
        extent residual {result.offset + length, result.length - length};
        status = this->fbt.insert(result.length, residual);
        if (status  < 0) return status;
    }

    target = {result.offset, length};

    auto list = this->get_spare_clusters();
    while (this->hdr.sc_count < this->hdr.sc_low_mark) {
        extent replacement;
        status = this->fbt.remove(length, replacement);
        if (status != status::OK) break;

        list[this->hdr.sc_count] = replacement;
    }

    return status::OK;
}

brufs::status brufs::brufs::allocate_tree_blocks(UNUSED size length, extent &target) {
    if (this->hdr.sc_count == 0) return status::E_NO_SPACE;
    --this->hdr.sc_count;

    auto list = this->get_spare_clusters();
    target = list[this->hdr.sc_count];

    return status::OK;
}

brufs::status brufs::brufs::free_blocks(const extent &ext) {
    auto fbt_block_size = this->hdr.fbt_rel_size * BLOCK_SIZE;

    if (this->hdr.sc_count < this->hdr.sc_high_mark && ext.length >= fbt_block_size) {
        auto list = this->get_spare_clusters();

        list[this->hdr.sc_count] = {ext.offset, fbt_block_size};

        extent residual {ext.offset + fbt_block_size, ext.length - fbt_block_size};
        if (residual.length > 0) {
            return this->fbt.insert(residual.length, residual);
        }

        return status::OK;
    }

    return this->fbt.insert(ext.length, ext);
}
