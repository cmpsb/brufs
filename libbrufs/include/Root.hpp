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

#include "internal.hpp"
#include "BmTree/btree-decl.hpp"
#include "RootHeader.hpp"
#include "InodeHeader.hpp"

namespace Brufs {

static const InodeId ROOT_DIR_INODE_ID = 1024;

class Root;
class Inode;
class File;
class Directory;

class InoTree : public BmTree::BmTree<InodeId, InodeHeader> {
private:
    Root &owner;
    Address *target;
public:
    InoTree(Brufs *fs, ::Brufs::Root &owner, Address *target, Size length) :
        BmTree::BmTree<InodeId, InodeHeader>(fs, *target, length), owner(owner), target(target)
    {}

    Status on_root_change(Address new_addr) override;
};

class Root {
    Brufs &fs;

    RootHeader header;

    InoTree it;
    InoTree ait;

    Status store();

    friend InoTree;

public:
    Root(Brufs &fs, const RootHeader &hdr);

    const RootHeader &get_header() { return this->header; }

    Brufs &get_fs() { return this->fs; }

    const RootHeader &get_header() const {
        return this->header;
    }

    Status init();

    InodeHeader *create_inode_header() const;
    void destroy_inode_header(InodeHeader *header) const;

    Status insert_inode(const InodeId &id, const InodeHeader *ino);
    Status find_inode(const InodeId &id, InodeHeader *ino);
    Status update_inode(const InodeId &id, const InodeHeader *ino);
    Status remove_inode(const InodeId &id, InodeHeader *ino);

    Status open_inode(const InodeId &id, Inode &inode);
    Status open_file(const InodeId &id, File &file);
    Status open_directory(const InodeId &id, Directory &dir);

    operator const RootHeader &() const {
        return this->header;
    }

    operator RootHeader &() {
        return this->header;
    }
};

inline Status InoTree::on_root_change(Address new_addr) {
    *this->target = new_addr;
    return this->owner.store();
}

}
