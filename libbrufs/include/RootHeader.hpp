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

#include <stdint.h>

#include "types.hpp"
#include "DirectoryEntry.hpp"
#include "String.hpp"
#include "InodeHeader.hpp"

namespace Brufs {

/**
 * A root in the filesystem.
 *
 * The filesystem may have multiple roots, each with their own principal owner.
 */
struct RootHeader {
    /**
     * The name of the folder
     *
     * NUL-terminated, so up to 255 characters long
     */
    char label[MAX_LABEL_LENGTH];

    /**
     * Flags
     *
     * Defaults to 0
     */
    uint64_t flags = 0;

    /**
     * The size of a full inode.
     */
    uint16_t inode_size = 128;

    /**
     * The size of just the inode header.
     */
    uint16_t inode_header_size = sizeof(InodeHeader);

    /**
     * The maximum length of a single extent in an IET.
     */
    uint32_t max_extent_length = 16 * 4096;

    /**
     * The offset the principal inode tree resides at.
     */
    uint64_t int_address = 0;

    /**
     * The offset the alternate inode tree resides at.
     */
    uint64_t ait_address = 0;

    Hash hash(const Hash seed = 14616742) const;

    void set_label(const String &label);

    bool operator==(const RootHeader &other) const {
        return memcmp(this, &other, sizeof(RootHeader));
    }
};
static_assert(std::is_standard_layout<RootHeader>::value, "the root structure must be standard-layout");
static_assert(sizeof(RootHeader) <= 512, "a root entry should fit in a block");
static_assert(sizeof(RootHeader) % 16 == 0, "a root entry should be 16-byte aligned");

}
