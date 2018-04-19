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

#include "internal.hpp"

const char *brufs::strerror(status eno) {
    if (eno >= status::E_ABSTIO_BASE && eno < 0) return "(see abstio)";

    switch (eno) {
        case E_INTERNAL: return "E_INTERNAL";
        case E_NO_MEM: return "E_NO_MEM";
        case E_DISK_TRUNCATED: return "E_DISK_TRUNCATED";
        case E_BAD_MAGIC: return "E_BAD_MAGIC";
        case E_FS_FROM_FUTURE: return "E_FS_FROM_FUTURE";
        case E_HEADER_TOO_BIG: return "E_HEADER_TOO_BIG";
        case E_CHECKSUM_MISMATCH: return "E_CHECKSUM_MISMATCH";
        case E_NO_SPACE: return "E_NO_SPACE";
        case E_WONT_FIT: return "E_WONT_FIT";
        case E_NOT_FOUND: return "E_NOT_FOUND";
        case E_TOO_MANY_RETRIES: return "E_TOO_MANY_RETRIES";
        case E_AT_MAX_LEVEL: return "E_AT_MAX_LEVEL";
        case E_CANT_ADOPT: return "E_CANT_ADOPT";
        case E_MISALIGNED: return "E_MISALIGNED";
        case E_ABSTIO_BASE: return "E_ABSTIO_BASE";
        case OK: return "OK";
        case RETRY: return "RETRY";
        default: return "(?)";
    }
}
