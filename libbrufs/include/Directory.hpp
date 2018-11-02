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

#include "types.hpp"
#include "Brufs.hpp"
#include "BmTree/btree-decl.hpp"
#include "Inode.hpp"
#include "Status.hpp"
#include "Vector.hpp"

namespace Brufs {

class Directory;

class FileEntryTree : public BmTree::BmTree<Hash, DirectoryEntry> {
private:
    Directory &dir;

public:
    FileEntryTree(Directory &dir);

    Status on_root_change(Address new_addr) override;
};

struct DirEnumerationHandle;

/**
 * A handle representing a directory on the file system.
 */
class Directory : public Inode {
private:
    Address *det_address_ptr() {
        return reinterpret_cast<Address *>(this->get_data());
    }

    Address &det_address() {
        return *this->det_address_ptr();
    }

    friend FileEntryTree;

public:
    using Inode::Inode;
    Directory(const Inode &other) : Inode(other) {}

    Directory &operator=(const Directory &other) {
        Inode::operator=(other);
        return *this;
    }

    Status init(const InodeId &id, const InodeHeader *hdr);
    Status destroy();

    /**
     * Looks up a directory entry by name.
     */
    Status look_up(const char *name, DirectoryEntry &target);

    Status insert(const DirectoryEntry &entry);

    Status update(const DirectoryEntry &entry);

    Status remove(const DirectoryEntry &entry);
    Status remove(const char *name, DirectoryEntry &entry);
    Status remove(const char *name);

    /**
     * Counts the entries in the directory.
     */
    SSize count();

    Status collect(Vector<DirectoryEntry> &entries);
};

inline FileEntryTree::FileEntryTree(Directory &dir) :
    BmTree::BmTree(
        &dir.get_root().get_fs(),
        dir.det_address(),
        dir.get_root().get_fs().get_header().cluster_size
    ),
    dir(dir)
{}

inline Status FileEntryTree::on_root_change(Address new_addr) {
    this->dir.det_address() = new_addr;
    return this->dir.store();
}

}
