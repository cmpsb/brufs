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

#include <math.h>
#include <string.h>

#include "PrettyPrint.hpp"

static constexpr unsigned int PPB_BUF_SIZE = 64;
static constexpr unsigned int PPII_BUF_SIZE = 40;

static constexpr unsigned int IEXEC  = 0000100;
static constexpr unsigned int IWRITE = 0000200;
static constexpr unsigned int IREAD  = 0000400;

static const char *suffixes[] = {
        "B",
        "kiB",
        "MiB",
        "GiB",
        "TiB",
        "PiB",
        "EiB",
        "ZiB",
        "YiB",
        "XiB",
        "WiB",
        "ViB",
        "UiB",
        "SiB",
        "HiB",
        "FiB",
};

Brufs::String Brufs::PrettyPrint::pp_size(__uint128_t bytes) const {
    const long double doubleBytes = bytes;
    const int magnitude = (bytes == 0) ? 0 : 
            (int) (log(doubleBytes) / log(1024));

    char buf[PPB_BUF_SIZE];
    snprintf(buf, PPB_BUF_SIZE, "%3.1Lf %s",
        doubleBytes / pow(1024, magnitude), suffixes[magnitude]
    );

    return String(buf);
}

Brufs::String Brufs::PrettyPrint::pp_inode_id(Brufs::InodeId inode_id) const {
    char buf[PPII_BUF_SIZE];
    snprintf(buf, PPII_BUF_SIZE, "%04hX:%04hX:%04hX:%04hX:%04hX:%04hX:%04hX:%04hX",
        (unsigned int) (inode_id >> 112) & 0xFFFF,
        (unsigned int) (inode_id >>  96) & 0xFFFF,
        (unsigned int) (inode_id >>  80) & 0xFFFF,
        (unsigned int) (inode_id >>  64) & 0xFFFF,
        (unsigned int) (inode_id >>  48) & 0xFFFF,
        (unsigned int) (inode_id >>  32) & 0xFFFF,
        (unsigned int) (inode_id >>  16) & 0xFFFF,
        (unsigned int) (inode_id >>   0) & 0xFFFF
    );

    return String(buf);
}

Brufs::String Brufs::PrettyPrint::pp_mode(bool is_dir, uint16_t mode) const {
    String line = "drwxrwxrwx";

    if (is_dir) {
        line[0] = 'd';
    } else {
        line[0] = '-';
    }

    const uint16_t iread  = IREAD  >> 6;
    const uint16_t iwrite = IWRITE >> 6;
    const uint16_t iexec  = IEXEC  >> 6;

    line[1] = ((mode >> 6) & iread ) ? 'r' : '-';
    line[2] = ((mode >> 6) & iwrite) ? 'w' : '-';
    line[3] = ((mode >> 6) & iexec ) ? 'x' : '-';

    line[4] = ((mode >> 3) & iread ) ? 'r' : '-';
    line[5] = ((mode >> 3) & iwrite) ? 'w' : '-';
    line[6] = ((mode >> 3) & iexec ) ? 'x' : '-';

    line[7] = ((mode >> 0) & iread ) ? 'r' : '-';
    line[8] = ((mode >> 0) & iwrite) ? 'w' : '-';
    line[9] = ((mode >> 0) & iexec ) ? 'x' : '-';

    return line;
}
