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

#include <cstdlib>
#include <cassert>

#include "internal.hpp"

static brufs::status verify_header(brufs::disk *disk, brufs::brufs *fs) {
    // Verify the magic signature
    auto diff = memcmp(fs->header.magic, brufs::MAGIC_STRING, brufs::MAGIC_STRING_LENGTH);
    if (diff) return brufs::status::E_BAD_MAGIC;

    // Check version compatibility
    brufs::version lib_version;
    brufs::get_version(&lib_version);
    if (lib_version < fs->header.version) {
        return brufs::status::E_FS_FROM_FUTURE;
    }

    // Verify the sanity of some key values
    if (fs->header.header_size > 4096) {
        return brufs::status::E_HEADER_TOO_BIG;
    }

    // Verify the header checksum
    void *buf = malloc(fs->header.header_size);
    if (!buf) return brufs::status::E_NO_MEM;

    auto status = brufs::dread(disk, buf, fs->header.header_size, 0);
    if (status < 0) {
        free(buf);
        return status;
    }

    auto hashable_header = static_cast<brufs::header *>(buf);
    hashable_header->checksum = 0;

    auto checksum = XXH64(hashable_header, fs->header.header_size, BRUFS_CHECKSUM_SEED);
    free(buf);

    if (checksum != fs->header.checksum) return brufs::status::E_CHECKSUM_MISMATCH;

    return brufs::status::OK;
}

brufs::brufs::brufs(brufs::disk *disk) : disk(disk) {
    brufs::ssize status;

    status = brufs::dread(disk, &fs->header, sizeof(fs->header), 0);
    if (status < 0) return;

    status = verify_header(*this);
    if (status < 0) return;

    this->root_ptr_index = static_cast<brufs::address *>(malloc(sizeof()))
}

brufs::status open(brufs::brufs *fs) :  {
    assert(fs->disk);
    assert(fs);

    brufs::ssize status;

    // Read the base structures of the file system
    status = brufs::dread(fs->disk, &fs->header, sizeof(fs->header), 0);
    if (status < 0) return static_cast<brufs::status>(status);

    status = verify_header(*fs);
    if (status < 0) return static_cast<brufs::status>(status);

    // Read the root pointer index
    fs->root_ptr_index = static_cast<brufs::address *>(malloc(fs->header.cluster_size));
    status = brufs::dread(fs->disk, fs->root_ptr_index, fs->header.cluster_size)
    if (status < 0) {
        free(fs->root_ptr_index);
        return static_cast<brufs::status>(status);
    }

    return brufs::status::OK;
}
