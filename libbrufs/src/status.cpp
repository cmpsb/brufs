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

#include "Status.hpp"

const char *Brufs::strerror(Status eno) {
    if (eno >= Status::E_ABSTIO_BASE && eno < 0) return "(see abstio)";

    switch (eno) {
        case E_INTERNAL: return "E_INTERNAL";
        case E_INVALID_ARGUMENT: return "E_INVALID_ARGUMENT";
        case E_NO_MEM: return "E_NO_MEM";
        case E_DISK_TRUNCATED: return "E_DISK_TRUNCATED";
        case E_BAD_MAGIC: return "E_BAD_MAGIC";
        case E_FS_FROM_FUTURE: return "E_FS_FROM_FUTURE";
        case E_HEADER_TOO_BIG: return "E_HEADER_TOO_BIG";
        case E_HEADER_TOO_SMALL: return "E_HEADER_TOO_SMALL";
        case E_CHECKSUM_MISMATCH: return "E_CHECKSUM_MISMATCH";
        case E_NO_SPACE: return "E_NO_SPACE";
        case E_WONT_FIT: return "E_WONT_FIT";
        case E_NOT_FOUND: return "E_NOT_FOUND";
        case E_TOO_MANY_RETRIES: return "E_TOO_MANY_RETRIES";
        case E_AT_MAX_LEVEL: return "E_AT_MAX_LEVEL";
        case E_CANT_ADOPT: return "E_CANT_ADOPT";
        case E_MISALIGNED: return "E_MISALIGNED";
        case E_NO_FBT: return "E_NO_FBT";
        case E_NO_RHT: return "E_NO_RHT";
        case E_EXISTS: return "E_EXISTS";
        case E_PILEUP: return "E_PILEUP";
        case E_BEYOND_EOF: return "E_BEYOND_EOF";
        case E_STOPPED: return "E_STOPPED";
        case E_WRONG_INODE_TYPE: return "E_WRONG_INODE_TYPE";
        case E_NOT_DIR: return "E_NOT_DIR";
        case E_IS_DIR: return "E_IS_DIR";
        case E_ABSTIO_BASE: return "E_ABSTIO_BASE";
        case OK: return "OK";
        case RETRY: return "RETRY";
        case STOP: return "STOP";
        default: return "(?)";
    }
}
