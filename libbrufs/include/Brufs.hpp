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

/**
 * A BmTree subclass updating the filesystem header on root changes.
 *
 * DO NOT USE EXCEPT INSIDE THE BRUFS CLASS!
 *
 * @tparam K the key type
 * @tparam V the value type
 */
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

/**
 * A Brufs filesystem instance, covering the entire device.
 *
 * This is the main handle for the entire libary. It represents the super-root of all roots,
 * contains the master header and provides free block management.
 */
class Brufs {
private:
    /**
     * The disk this filesystem is on.
     */
    Disk *dsk;

    union {
        /**
         * The filesystem header as a raw set of bytes.
         *
         * This field is used by the free block management code to store the reserved extent list.
         */
        char *raw_header;

        /**
         * The filesystem header.
         */
        Header *hdr;
    };

    /**
     * The free blocks tree.
     *
     * This tree stores free extents on the disk indexed by their size.
     */
    FsCTree<Size, Extent> fbt;

    /**
     * The root hash table.
     *
     * This tree represents a hash table containing all roots.
     */
    FsCTree<Hash, RootHeader> rht;

    /**
     * The file system's load status.
     *
     * Modified as soon as the filesystem is loaded and never after.
     */
    Status stt = Status::OK;

    /**
     * Returns the start of the list of reserved clusters.
     *
     * The size of the list is determined by this->hdr->sc_count.
     *
     * @return the reserved clusters
     */
    Extent *get_spare_clusters() {
        return reinterpret_cast<Extent *>(this->raw_header + this->hdr->header_size);
    }

    /**
     * Writes the entire header to disk.
     *
     * @return the status
     */
    Status store_header();

    friend FsCTree<Size, Extent>;
    friend FsCTree<Hash, RootHeader>;

public:
    /**
     * Opens a new Brufs instance.
     *
     * @param dsk the disk the filesystem runs on
     */
    Brufs(Disk *dsk);

    /**
     * Closes a Brufs instance.
     */
    ~Brufs();

    /**
     * Returns the filesystem loading status.
     *
     * This will always return the same value throughout the lifetime of the instance.
     *
     * @return the status
     */
    Status get_status() const { return this->stt; }

    /**
     * Returns the disk the filesystem is stored on.
     *
     * @return the disk
     */
    Disk *get_disk() { return this->dsk; }

    /**
     * Returns the header of the filesystem.
     *
     * @return the header
     */
    const Header &get_header() const { return *this->hdr; }

    /**
     * Initializes (formats) the filesystem.
     *
     * The following fields are copied from the prototype to the new filesystem's actual header:
     * * cluster_size_exp
     * * sc_low_mark
     * * sc_high_mark
     *
     * The other fields are ignored; they are set automatically during initalization.
     *
     * @param protoheader a prototype for the filesystem header
     *
     * @return the status
     */
    Status init(Header &protoheader);

    /**
     * Allocates a number of free blocks.
     *
     * The allocated extent will be exactly the requested size. It may not be possible to allocate
     * an extent of this size, in this case the function will return E_NO_SPACE.
     *
     * If the size is not 512 nor a multiple of the cluster size, the function will
     * return E_MISALIGNED.
     *
     * @param length the number of bytes to allocate
     * @param target where to store the offset and length of the allocated extent
     *
     * @return the status
     */
    Status allocate_blocks(Size length, Extent &target);

    /**
     * Frees an extent.
     *
     * Double-free is not checked.
     *
     * @param extent the extent to free
     *
     * @return the status
     */
    Status free_blocks(const Extent &extent);

    /**
     * Counts the size of various areas and structures covering the entire filesystem.
     *
     * @param reserved the number of bytes in the spare cluster list
     * @param available the number of bytes left available in the system
     * @param extents the number of distinct extents in the free blocks tree
     * @param in_fbt the number of bytes used by the free blocks tree
     *
     * @return the status
     */
    Status count_free_blocks(Size &reserved, Size &available, Size &extents, Size &in_fbt);

    /**
     * Allocates a free cluster from the spare cluster list for use in the free blocks tree.
     *
     * @param length the length of the cluster (unused, the global cluster size is used instead)
     * @param target where to store the extent information
     *
     * @return the status
     */
    Status allocate_tree_blocks(Size length, Extent &target);

    /**
     * Counts the number of roots in the filesystem.
     *
     * @return the number of roots, or a negative status
     */
    SSize count_roots();

    /**
     * Collects the root in the filesystem.
     *
     * @param collection the array to fill
     * @param count the number of elements available in the array
     *
     * @return the number of roots, or a negative status
     */
    int collect_roots(RootHeader *collection, Size count);

    /**
     * Locates a root in the filesystem.
     *
     * @param name the name of the root
     * @param target where to store the root header, if one was found
     *
     * @return the status
     */
    Status find_root(const char *name, RootHeader &target);

    /**
     * Inserts a root into the filesystem.
     *
     * Two roots with the same name are not allowed, in which case the function returns E_EXISTS.
     *
     * @param root the root header to insert
     *
     * @return the status
     */
    Status add_root(const RootHeader &root);

    /**
     * Updates a root in the filesystem.
     *
     * The root must exist before this function is called, otherwise E_NOT_FOUND will be returned.
     *
     * @param rt the root to update
     *
     * @return the status
     */
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
