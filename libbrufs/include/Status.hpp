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

#include <limits.h>

namespace Brufs {

/**
 * A status code.
 * Negative values indicate an error, 0 indicates nominal success.
 */
enum Status {
    /**
     * An internal error that can't be explained.
     * The library should try to NEVER generate this status.
     */
    E_INTERNAL = INT_MIN,

    /**
     * The argument was invalid (e.g., null).
     */
    E_INVALID_ARGUMENT,

    /**
     * A dynamic memory allocated failed.
     */
    E_NO_MEM,

    /**
     * The disk ended before all data could be written.
     */
    E_DISK_TRUNCATED,

    /**
     * The filesystem header has a bad signature.
     */
    E_BAD_MAGIC,

    /**
     * The filesystem version is too high for this library.
     */
    E_FS_FROM_FUTURE,

    /**
     * The filesystem header is too big (> 4096 bytes, minus the free block cache)
     */
    E_HEADER_TOO_BIG,

    /**
     * The filesystem header is too small for all values.
     */
    E_HEADER_TOO_SMALL,

    /**
     * THe checksum of one or more blocks does not match the expected value.
     */
    E_CHECKSUM_MISMATCH,

    /**
     * Not enough space left on device.
     */
    E_NO_SPACE,

    /**
     * There is enough space, but the filesystem is too fragmented.
     */
    E_WONT_FIT,

    /**
     * The entity (key, directory, file) could not be found.
     */
    E_NOT_FOUND,

    /**
     * The system returned a RETRY status too often.
     */
    E_TOO_MANY_RETRIES,

    /**
     * The tree grew to its maximum level.
     */
    E_AT_MAX_LEVEL,

    /**
     * A soft error indicating that the neighboring node can't adopt the given node.
     * This code should not be encountered outside the Bm+tree library.
     */
    E_CANT_ADOPT,

    /**
     * The data structure or its size is misaligned or causes other structures to be misaligned.
     */
    E_MISALIGNED,

    /**
     * Core datastructures are missing.
     */
    E_NO_FBT,
    E_NO_RHT,

    /**
     * The resource already exists.
     */
    E_EXISTS,

    /**
     * Too many collisions.
     */
    E_PILEUP,

    /**
     * The file offset is beyonde the end of the file.
     */
    E_BEYOND_EOF,

    /**
     * A callback requested to operation to be stopped, but this left the system in an unstable
     * state.
     */
    E_STOPPED,

    /**
     * The requested operation is not valid for the inode's type.
     */
    E_WRONG_INODE_TYPE,

    /**
     * The path does not contain a root.
     */
    E_NO_ROOT,

    /**
     * Not a real error, but rather the lowest possible I/O abstraction status code
     */
    E_ABSTIO_BASE = INT_MIN >> 1,

    /**
     * Success.
     */
    OK = 0,

    /**
     * An exceptional situation happened, but the user should keep retrying until the operation
     * succeeds.
     */
    RETRY,

    /**
     * A request to stop calling that function.
     */
    STOP
};

const char *strerror(Status eno);

}
