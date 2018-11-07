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
#include "Inode.hpp"
#include "DataExtent.hpp"

namespace Brufs {

class File;

class InodeExtentTree : public BmTree::BmTree<Offset, DataExtent> {
private:
    File &file;
public:
    InodeExtentTree(File &file);

    Status on_root_change(Address new_addr) override;
};

class File : public Inode {
private:
    Address &iet_address() {
        return *((Address *) this->get_data());
    }

    friend InodeExtentTree;

    Status resize_small_to_small(Size old_size, Size new_size);
    Status resize_small_to_big  (Size old_size, Size new_size);
    Status resize_big_to_small  (Size old_size, Size new_size);
    Status resize_big_to_big    (Size old_size, Size new_size);

public:
    using Inode::Inode;
    File(const Inode &other) : Inode(other) {}

    File &operator=(const File &other) {
        Inode::operator=(other);

        return *this;
    }

    Status destroy() override;

    Status truncate(Size length);
    Status empty();
    SSize write(const void *buf, Size count, Offset offset);
    SSize read(void *buf, Size count, Offset offset);

    File &set_size(Size new_size) {
        this->get_header()->file_size = new_size;
        return *this;
    }

    Size get_size() const {
        return this->get_header()->file_size;
    }
};

inline InodeExtentTree::InodeExtentTree(File &file) :
    BmTree::BmTree(
        &file.get_root().get_fs(),
        file.iet_address(),
        file.get_root().get_fs().get_header().cluster_size
    ),
    file(file)
{}

inline Status InodeExtentTree::on_root_change(Address new_addr) {
    this->file.iet_address() = new_addr;
    return this->file.store();
}

}
