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

#include "types.hpp"
#include "io.hpp"
#include "Disk.hpp"
#include "Status.hpp"

/**
 * Reads a number of bytes from a certain offset from disk.
 * Guarantees that exactly `count` bytes are read.
 *
 * @param dsk the disk to read from
 * @param buf the buffer to store the bytes in
 * @param count the number of bytes to read
 * @param offset the offset to read from
 * @return either `count`, BRUFS_E_DISK_TRUNCATED if not enough bytes could be read or any error
 *   returned by the disk I/O abstraction layer
 */
Brufs::SSize Brufs::dread(Disk *dsk, void *buf, Size count, Address offset) {
    char *cbuf = static_cast<char *>(buf);

    Size total = 0;
    while (total < count) {
        SSize num_read = dsk->io->read(cbuf + total, count - total, offset + total);
        if (num_read == 0) break;
        if (num_read < 0) return num_read;

        total += num_read;
    }

    if (total < count) {
        return Status::E_DISK_TRUNCATED;
    }

    return static_cast<SSize>(total);
}

/**
 * Writes a number of bytes from a certain offset to disk.
 * Guarantees that exactly `count` bytes are written.
 *
 * @param dsk the disk to write to
 * @param buf the buffer to read the bytes from
 * @param count the number of bytes to write
 * @param offset the offset to write from
 * @return either `count`, BRUFS_E_DISK_TRUNCATED if not enough bytes could be written or any error
 *   returned by the disk I/O abstraction layer
 */
Brufs::SSize Brufs::dwrite(Disk *dsk, const void *buf, Size count, Address offset) {
    const char *cbuf = static_cast<const char *>(buf);

    Size total = 0;
    while (total < count) {
        SSize num_written = dsk->io->write(cbuf + total, count - total, offset + total);
        if (num_written == 0) break;
        if (num_written < 0) return num_written;

        total += num_written;
    }

    if (total < count) {
        return Status::E_DISK_TRUNCATED;
    }

    return static_cast<SSize>(total);
}
