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
#include "String.hpp"

namespace Brufs {

/**
 * Pretty print utility functions.
 */
class PrettyPrint {
public:
    /**
     * Pretty-prints a size in bytes.
     * 
     * The size is scaled to between 1 and 3 integer digits and a single-digit fractional part.
     * The size is then prepended to the appropriate IEC-compliant suffix.
     * 
     * @param bytes the size to pretty-print
     * @return the pretty-printed size
     */
    String pp_size(__uint128_t bytes) const;

    /**
     * Pretty-prints an inode ID as a series of 16-bit words separated by colons.
     * 
     * @param id the ID to pretty-print
     * @return the pretty-printed ID
     */
    String pp_inode_id(InodeId id) const;

    /**
     * Pretty-prints an inode mode as a UNIX-like mode string.
     * 
     * @param is_dir iff true, the mode is treated as a mode for a directory
     * @param mode the mode to pretty-print
     * 
     * @return the pretty-printed mode
     */
    String pp_mode(bool is_dir, uint16_t mode) const;
};

}
