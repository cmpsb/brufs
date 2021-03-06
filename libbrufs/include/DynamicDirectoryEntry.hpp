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

#include "DirectoryEntry.hpp"

namespace Brufs {

/**
 * A directory entry that uses dynamically-sized storage as opposed to the fixed size approach
 * of DirectoryEntry.
 */
class DynamicDirectoryEntry {
private:
    String label;
    InodeId inode_id;

public:
    DynamicDirectoryEntry() = default;
    DynamicDirectoryEntry(const DynamicDirectoryEntry &other) = default;

    DynamicDirectoryEntry(const DirectoryEntry &entry) :
        label(entry.get_label()), inode_id(entry.inode_id)
    {}

    DynamicDirectoryEntry(const String &label, const InodeId &inode_id) :
        label(label), inode_id(inode_id)
    {}

    const String &get_label() const {
        return this->label;
    }

    const InodeId &get_inode_id() const {
        return this->inode_id;
    }

    operator DirectoryEntry() const {
        return {this->label, this->inode_id};
    }
};

}
