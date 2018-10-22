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
#pragma once

#include <type_traits>

#include <cstdint>

#include "Extent.hpp"
#include "Version.hpp"

namespace Brufs {

/**
 * The magic byte sequence identifying a Brufs installation.
 */
static const char *const MAGIC_STRING = "BRUTAFS\nBINARY\n";

/**
 * The full length of the magic string in bytes.
 */
static const Size MAGIC_STRING_LENGTH = 16;

/**
 * The master header of the filesystem.
 */
struct Header {
    /**
     * Magic string identifying the partition as Brufs
     */
    uint8_t magic[MAGIC_STRING_LENGTH];

    /**
     * Version info
     */
    Version ver;

    /**
     * The size of the header in bytes
     */
    uint32_t header_size;

    /**
     * xxHash64 of the filesystem header plus whatever data is present up to header_size bytes,
     * byte-wise, with this field 0
     */
    uint64_t checksum;

    /**
     * The size of a single cluster in bytes.
     */
    uint32_t cluster_size;

    /**
     * The binary logarithm of above size.
     */
    uint8_t cluster_size_exp;

    /**
     * The minimum number of spare clusters to reserve for the free blocks tree.
     */
    uint8_t sc_low_mark;

    /**
     * The maximum amount of spare clusters to reserve.
     */
    uint8_t sc_high_mark;

    /**
     * The current amount of spare clusters on disk.
     */
    uint8_t sc_count;

    /**
     * The total number of 512-byte blocks in the filesystem.
     */
    uint64_t num_blocks;

    /**
     * The starting address of the free blocks tree.
     */
    uint64_t fbt_address;

    /**
     * The starting address of the root entry list
     */
    uint64_t rht_address;

    /**
     * Flags
     * 
     * Defaults to 0
     */
    uint64_t flags;

    int validate(void *disk) const;
};
static_assert(std::is_standard_layout<Header>::value, "the fs header must be standard-layout");
static_assert(sizeof(Header) <= (4096 - 16 * sizeof(Extent)), "the fs header should fit in 4k");

}
