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
#include "Timestamp.hpp"
#include "InodeType.hpp"

namespace Brufs {

/**
 * Settings for inodes.
 *
 * The values in this enum are the bit positions of the flags; one can, for example, set the
 * ZERO_AT_DELETION flag as follows:
 *
 * \code{.cpp}
 * inode_header.flags = (1 << ZERO_AT_DELETION);
 * \endcode
 */
enum InodeFlag {
    NO_SPARSE,
    ZERO_AT_DELETION
};

/**
 * The fields common to all inodes.
 */
struct InodeHeader {
    /**
     * The timestamp the file was created
     *
     * Should not change after the inode is allocated
     */
    Timestamp created;

    /**
     * The timestamp the file was last modified
     */
    Timestamp last_modified;

    /**
     * The ID of the owner.
     */
    OwnerId owner;

    /**
     * The ID of the group.
     */
    OwnerId group;

    /**
     * The number of hard links referencing this inode
     */
    uint16_t num_links;

    /**
     * Inode or file type
     */
    uint16_t type;

    /**
     * Flags
     *
     * Defaults to 0
     */
    uint16_t flags;

    /**
     * The file access mode.
     */
    uint16_t mode;

    /**
     * The size of the file in bytes
     */
    Size file_size;

    /**
     * Checksum of this inode, with this field 0
     */
    Hash checksum;

    bool test_flag(const InodeFlag index) const {
        return this->flags & (1 << index);
    }

    void set_flag(const InodeFlag index, const bool value) {
        auto bit = (1 << index);
        this->flags = (this->flags & ~bit) | (value * bit);
    }
};
static_assert(
    std::is_standard_layout<InodeHeader>::value, "the inode header must be standard-layout"
);
static_assert(
    sizeof(InodeHeader) < 128,  "inode headers should be smaller than 128 bytes"
);

}
