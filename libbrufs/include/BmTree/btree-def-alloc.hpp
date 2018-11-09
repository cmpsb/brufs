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

#include "types.hpp"
#include "Brufs.hpp"
#include "Extent.hpp"

namespace Brufs { namespace BmTree {

/**
 * Allocates blocks using a separate pool of blocks where the free blocks tree should pull
 * free blocks from, to prevent deadlocks.
 */
static inline Status ALLOC_FBT_BLOCK(Brufs &fs, Size length, Extent &target) {
    return fs.allocate_tree_blocks(length, target);
}

/**
 * Will never allocate a new block, returning E_NO_SPACE instead.
 */
static inline Status ALLOC_NEVER(Brufs &fs, Size length, Extent &target) {
    (void) fs;
    (void) length;
    (void) target;
    return Status::E_NO_SPACE;
}

static inline Status ALLOC_NORMAL(Brufs &fs, Size length, Extent &target) {
    return fs.allocate_blocks(length, target);
}

static inline Status DEALLOC_NORMAL(Brufs &fs, const Extent &ext) {
    return fs.free_blocks(ext);
}

}}
