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
#include <cstring>

#include "xxhash/xxhash.h"

#include "io.hpp"
#include "internal.hpp"
#include "Header.hpp"
#include "Status.hpp"
#include "Version.hpp"

int Brufs::Header::validate(void *dsk) const {
    // Verify the magic signature
    auto diff = memcmp(this->magic, MAGIC_STRING, MAGIC_STRING_LENGTH);
    if (diff) return Status::E_BAD_MAGIC;

    // Check version compatibility
    Version lib_version = Version::get();
    if (lib_version < this->ver) {
        return Status::E_FS_FROM_FUTURE;
    }

    // Verify the sanity of some key values
    if (this->header_size > (4096 - 16 * sizeof(Extent))) {
        return Status::E_HEADER_TOO_BIG;
    }

    if (this->header_size < sizeof(Header)) {
        return Status::E_HEADER_TOO_SMALL;
    }

    if (!is_power_of_two(this->cluster_size)) {
        return Status::E_MISALIGNED;
    }

    if (this->fbt_address == 0) {
        return Status::E_NO_FBT;
    }

    if (this->rht_address == 0) {
        return Status::E_NO_RHT;
    }

    // Verify the header checksum
    void *buf = malloc(this->header_size);
    if (!buf) return Status::E_NO_MEM;

    auto status = dread(static_cast<Disk *>(dsk), buf, this->header_size, 0);
    if (status < 0) {
        free(buf);
        return status;
    }

    auto hashable_header = static_cast<Header *>(buf);
    hashable_header->checksum = 0;

    auto checksum = XXH64(hashable_header, this->header_size, CHECKSUM_SEED);
    free(buf);

    if (checksum != this->checksum) return Status::E_CHECKSUM_MISMATCH;

    return Status::OK;
}
