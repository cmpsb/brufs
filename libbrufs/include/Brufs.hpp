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
#include "Disk.hpp"
#include "Header.hpp"
#include "Inode.hpp"
#include "Root.hpp"
#include "RootHeader.hpp"
#include "Status.hpp"
#include "Version.hpp"
#include "BmTree/btree-decl.hpp"

namespace Brufs {

class File;
class Directory;

void get_version(Version &version);

template <typename K, typename V>
class FsCTree : public BmTree::BmTree<K, V> {
private:
    Brufs *fs;
    Address *target;
public:
    FsCTree(Brufs *fs, Address *target, BmTree::Allocator alloc = BmTree::ALLOC_NORMAL) : 
        BmTree::BmTree<K, V>(fs, 0, alloc), fs(fs), target(target)
    {}

    FsCTree(Brufs *fs, Address *target, Size length) :
        BmTree::BmTree<K, V>(fs, length), fs(fs), target(target)
    {}

    void set_target(Address *target) {
        this->target = target;
    }

    Status on_root_change(Address new_addr) override;
};

class Brufs {
private:
    Disk *dsk;

    union {
        char *raw_header;
        Header *hdr;
    };

    FsCTree<Size, Extent> fbt;
    FsCTree<Hash, RootHeader> rht;

    Status stt = Status::OK;

    Extent *get_spare_clusters() { 
        return reinterpret_cast<Extent *>(this->raw_header + this->hdr->header_size);
    }

    Status store_header();

    friend FsCTree<Size, Extent>;
    friend FsCTree<Hash, RootHeader>;

public:
    Brufs(Disk *dsk);
    ~Brufs();

    Status get_status() const { return this->stt; }

    Disk *get_disk() { return this->dsk; }

    const Header &get_header() const { return *this->hdr; }

    Status init(Header &protoheader);

    Status allocate_blocks(Size length, Extent &target);
    Status free_blocks(const Extent &extent);

    Status count_free_blocks(Size &reserved, Size &available, Size &extents, Size &in_fbt);

    Status allocate_tree_blocks(Size length, Extent &target);

    SSize count_roots();
    int collect_roots(RootHeader *collection, Size count);

    Status find_root(const char *name, RootHeader &target);
    Status add_root(const RootHeader &target);
    Status update_root(const RootHeader &rt);
};

template <typename K, typename V>
Status FsCTree<K, V>::on_root_change(Address new_addr) {
    *this->target = new_addr;
    return this->fs->store_header();
}

}

#include "BmTree/btree-def-alloc.hpp"
#include "BmTree/btree-def-node.hpp"
#include "BmTree/btree-def-container.hpp"
