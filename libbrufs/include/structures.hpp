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
#include <cstdlib>
#include <cstdint>
#include <cstring>

#include "types.hpp"

namespace brufs {

static const char * const MAGIC_STRING = "BRUTAFS\nBINARY\n";
static const size MAGIC_STRING_LENGTH = 16;

static const hash CHECKSUM_SEED = 14616742;

static const size MAX_LABEL_LENGTH = 256;

static const size BLOCK_SIZE = 512;

static inline bool is_valid_size(size size) {
    return (size & 511) == 0;
}

template <typename T>
static inline bool is_power_of_two(T v) {
    // https://graphics.stanford.edu/~seander/bithacks.html#DetermineIfPowerOf2
    return v && !(v & (v - 1));
}

struct version {
    uint8_t major;
    uint8_t minor;
    uint16_t patch;

    static version get();

    int compare(const version &other) const;

    auto operator<(const version &other) const { return this->compare(other) < 0; }
    auto operator>(const version &other) const { return this->compare(other) > 0; }
    
    auto operator==(const version &other) const { return this->compare(other) == 0; }
    auto operator!=(const version &other) const { return this->compare(other) != 0; }

    auto operator<=(const version &other) const { return *this == other || *this < other; }
    auto operator>=(const version &other) const { return *this == other || *this > other; }

    int to_string(char *buf, size len) const;
};
static_assert(
    std::is_standard_layout<version>::value, "the version structure must be standard-layout"
);
static_assert(sizeof(version) == 4, "the version structure must be exactly 4 bytes long");

/**
 * An extent describing the start of a contiguous bunch of data and how long it is
 */
struct extent {
    /**
     * A magic address that indicates that the free list ends here.
     */
    static const address END;

    /**
     * The start LBA of the extent
     */
    address offset;

    /**
     * The length of the extent in blocks
     */
    size length;

    extent() = default;

    extent(address offset, size length) : offset(offset), length(length) {}

    auto operator==(const extent &other) const {
        return this->offset == other.offset && this->length == other.length;
    }
};
static_assert(
    std::is_standard_layout<extent>::value, "the extent structure must be standard-layout"
);

struct checked_extent {
    /**
     * The extent itself.
     */
    extent ext;

    /**
     * The checksum of the data in the extent
     */
    hash checksum;
};
static_assert(
    std::is_standard_layout<checked_extent>::value, 
    "the checked extent structure must be standard-layout"
);

struct header {
    /**
     * Magic string identifying the partition as Brufs
     */
    uint8_t magic[MAGIC_STRING_LENGTH];

    /**
     * Version info
     */
    version ver;

    /**
     * The size of the header in bytes
     */
    uint32_t header_size;

    /**
     * xxHash64 of the filesystem header plus whatever data is present up to header_size bytes,
     * byte-wise, with this field 0
     */
    uint64_t checksum;

    uint32_t cluster_size;
    uint8_t cluster_size_exp;
    uint8_t unused1[3];
    uint64_t num_blocks;

    uint8_t sc_low_mark;
    uint8_t sc_high_mark;
    uint8_t sc_count;
    uint8_t unused2;

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
static_assert(std::is_standard_layout<header>::value, "the fs header must be standard-layout");
static_assert(
    sizeof(header) <= (4096 - 16 * sizeof(struct extent)), "the fs header should fit in 4k"
);

/**
 * A root in the filesystem
 * 
 * The filesystem may have multiple roots, each with their own principal owner.
 */
struct root_header {
    /**
     * The name of the folder
     * 
     * NUL-terminated, so up to 255 characters long
     */
    char label[MAX_LABEL_LENGTH];

    /**
     * The principal owner of the root.
     */
    __uint128_t owner;

    /**
     * Flags
     * 
     * Defaults to 0
     */
    uint64_t flags;

    uint64_t int_address;
    uint64_t ait_address;
};
static_assert(std::is_standard_layout<root_header>::value, "the root structure must be standard-layout");
static_assert(sizeof(root_header) <= 512, "a root entry should fit in a block");
static_assert(sizeof(root_header) % 16 == 0, "a root entry should be 16-byte aligned");

/**
 * An entry in the big root pointer hash table.
 */
struct rootptr {
    /**
     * A magic address that indicates that no other possible matches could follow.
     */
    static const address END;

    /**
     * A magic address that indicates that this entry was deleted, 
     * but that other matches may follow.
     */
    static const address DELETED;

    /**
     * The hash of the label for fast collision rejection.
     */
    hash label_hash;

    /**
     * The address of the root block.
     */
    address location;
};
static_assert(
    std::is_standard_layout<rootptr>::value, "the root pointer structure must be standard-layout"
);
static_assert(sizeof(rootptr) == 16, "a root pointer should be exactly 16 bytes");

/**
 * A directory entry
 * 
 * Entries are variable in size, due to the 
 * 
 */
struct dirent {
    /**
     * The name of the entry
     * 
     * NUL-terminated, so up to 255 characters long
     */
    char label[MAX_LABEL_LENGTH];

    /**
     * The hash of the label for fast collision rejection
     */
    hash label_hash;
};
static_assert(
    std::is_standard_layout<dirent>::value, "the directory entry structure must be standard-layout"
);
static_assert(sizeof(dirent) <= 512, "a directory entry should fit in a block");

/**
 * A timestamp
 * 
 */
struct timestamp {
    uint64_t seconds;
    uint64_t nanoseconds;
};
static_assert(
    std::is_standard_layout<timestamp>::value, "the timestamp structure must be standard-layout"
);

/**
 * The fields common to all inodes.
 */
struct inode_header {
    /**
     * The timestamp the file was created
     * 
     * Should not change after the inode is allocated
     */
    timestamp created;

    /**
     * The timestamp the file was last modified
     */
    timestamp last_modified;

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
    uint32_t flags;

    /**
     * The size of the file in bytes
     */
    size fileSize;

    /**
     * Checksum of this inode, with this field 0
     */
    hash checksum;
};
static_assert(
    std::is_standard_layout<inode_header>::value, "the inode header must be standard-layout"
);

/**
 * A inode, containing the most important metadata of a file
 * 
 */
struct inode {
    inode_header header;

    /**
     * The list of extents the file occupies, or the file data if it fits
     * 
     * How many extents there are depends on the size of an inode, as determined by the 
     * cluster group header
     * 
     * If the file is small enough to fit whithin this space, this will be the raw bytes instead
     * Otherwise, these bytes point to the blocks on disk where the file is stored
     * If more space is required (the indicated file size in the inode header > the sum of all 
     * extent sizes), then the size of the last extent will be <= 128 with the offset pointing
     * to a tree containing the rest of the extents. The checksum remains just that, only
     * now covering the block(s) the tree is occupying.
     */
    extent extents[1];
};
static_assert(
    std::is_standard_layout<inode>::value, "the inode structure must be standard-layout"
);

}
