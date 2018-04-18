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

#include <cstdint>

#include "structures.hpp"
#include "status.hpp"

namespace brufs {

using ssize = long long;
using offset = long long;

class abstio {
public:
    virtual ~abstio() = 0;

    /**
     * Reads at most `count` bytes from the disk, starting from offset `offset`.
     * This function should be able to handle reads outside the disk's boundaries by returning an
     * error.
     * 
     * @param buf the buffer to store the read bytes in
     * @param count the number of bytes to read
     * @param offset the offset from which to start reading
     * 
     * @return the actual number of bytes read, 0 on EOD, or any status code on error
     */
    virtual ssize read(void *buf, size count, address offset) const = 0;

    /**
     * Writes at most `count` bytes to the disk, starting from offset `offset`.
     * This function should be able to handle writes outside the disk's boundaries by returning
     * an error.
     * 
     * @param buf the buffer to read the bytes from
     * @param count the number of bytes to write
     * @param offset the offset from which to start writing
     * 
     * @return the actual number of bytes written, or any status code on error
     */
    virtual ssize write(const void *buf, size count, address offset) = 0;

    /**
     * Returns a string describing the given status code.
     * 
     * @param eno the status code
     * 
     * @return the human-readable string
     */
    virtual const char *strstatus(ssize eno) const = 0;

    /**
     * Returns the size, in bytes, of the entire disk.
     *
     * @return the size
     */
    virtual size get_size() const = 0;
};

struct disk {
    abstio *io;

    disk(abstio *io) : io(io) {}
};

class brufs {
private:
    disk *dsk;

    header hdr;

    address *root_ptr_index;
    rootptr **root_ptrs;

    address *free_extent_index;

    uint16_t blocks_per_cluster;

    status init_free_list();
    status open_free_list();

public:
    brufs(disk *dsk) : dsk(dsk) {}

    disk *get_disk() { return this->dsk; }

    status init();

    status allocate_blocks(size length, extent &target);
    status free_blocks(const extent &extent);

    status allocate_tree_blocks(size length, extent &target);
};

}
