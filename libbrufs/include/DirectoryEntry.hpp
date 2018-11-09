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

#include <type_traits>
#include <cstdlib>
#include <cstring>

#include "types.hpp"
#include "xxhash/xxhash.h"
#include "String.hpp"

namespace Brufs {

static constexpr Size MAX_LABEL_LENGTH = 256;

/**
 * A directory entry
 *
 * Entries are variable in size, due to the
 *
 */
struct DirectoryEntry {
    /**
     * The name of the entry
     *
     * At most MAX_LABEL_LENGTH characters long, with a NUL terminator if it's shorter than that.
     */
    char label[MAX_LABEL_LENGTH];

    InodeId inode_id;

    void set_label(const String &label) {
        strncpy(this->label, label.c_str(), MAX_LABEL_LENGTH);
    }

    String get_label() const {
        char fit_label[MAX_LABEL_LENGTH + 1];
        strncpy(fit_label, this->label, MAX_LABEL_LENGTH);

        return fit_label;
    }

    Hash hash(const Hash seed = 14616742) const {
        char lbl[MAX_LABEL_LENGTH + 1];
        memcpy(lbl, this->label, MAX_LABEL_LENGTH);
        lbl[MAX_LABEL_LENGTH] = 0;

        return XXH64(this->label, strlen(lbl), seed);
    }
};
static_assert(
    std::is_standard_layout<DirectoryEntry>::value, "the directory entry structure must be standard-layout"
);
static_assert(sizeof(DirectoryEntry) <= 512, "a directory entry should fit in a block");

}
