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

namespace Brufs {

class AbstIO {
public:
    virtual ~AbstIO() = 0;

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
    virtual SSize read(void *buf, Size count, Address offset) const = 0;

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
    virtual SSize write(const void *buf, Size count, Address offset) = 0;

    /**
     * Returns a string describing the given Status code.
     *
     * @param eno the Status code
     *
     * @return the human-readable string
     */
    virtual const char *strstatus(SSize eno) const = 0;

    /**
     * Returns the size, in bytes, of the entire disk.
     *
     * @return the size
     */
    virtual Size get_size() const = 0;
};

}
