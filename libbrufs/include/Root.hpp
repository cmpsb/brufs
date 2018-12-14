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

#include "internal.hpp"
#include "BmTree/btree-decl.hpp"
#include "RootHeader.hpp"
#include "InodeHeader.hpp"
#include "Path.hpp"
#include "InodeHeaderBuilder.hpp"

namespace Brufs {

static const InodeId ROOT_DIR_INODE_ID = 1024;

class Root;
class Inode;
class File;
class Directory;

/**
 * A B+tree containing inodes belonging to a root.
 */
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

/**
 * A root in the filesystem.
 */
class Root {
    /**
     * The filesystem the root belongs to.
     */
    Brufs &fs;

    /**
     * The on-disk structure describing the header.
     */
    RootHeader header;

    /**
     * The main inode tree.
     */
    InoTree it;

    /**
     * The alternate inode tree, containing extended attributes for example.
     */
    InoTree ait;

    friend InoTree;

    /**
     * Enables or disables automatic storage upon modification.
     */
    bool enable_store = true;

public:
    /**
     * Constructs a new root handle.
     * 
     * @param fs the filesystem the root is on
     * @param hdr the on-disk header of the root
     */
    Root(Brufs &fs, const RootHeader &hdr);

    // Roots are non-copyable
    Root(const Root &other) = delete;
    Root &operator=(const Root &other) = delete;

    /**
     * Returns the on-disk structure representing the header.
     * 
     * @return the on-disk structure
     */
    const RootHeader &get_header() { return this->header; }
    const RootHeader &get_header() const { return this->header; }

    /**
     * Returns the filesystem the root is on.
     * 
     * @return the filesystem
     */
    Brufs &get_fs() { return this->fs; }
    const Brufs &get_fs() const { return this->fs; }

    /**
     * Initializes the root.
     * 
     * @param ihb an inode header builder used to construct the actual root directory
     * 
     * @return the status return code
     */
    Status init(const InodeHeaderBuilder &ihb = InodeHeaderBuilder());

    /**
     * Writes the root to disk.
     * 
     * @return the status return code
     */
    Status store();

    /**
     * Creates a temporary inode header of the correct size.
     * 
     * @return a heap-allocated header
     */
    InodeHeader *create_inode_header() const;

    /**
     * Destroys an allocated inode header.
     * 
     * @param header the header to destroy
     */
    void destroy_inode_header(InodeHeader *header) const;

    /**
     * Inserts an inode into the root.
     * 
     * @param id the inode ID of the inode to add
     * @param ino the inode to add
     * 
     * @return the status return code
     */
    Status insert_inode(const InodeId &id, const InodeHeader *ino);

    /**
     * Looks up an inode in the root.
     * 
     * @param id the inode ID of the inode to look for
     * @param ino where to store the found inode header
     * 
     * @return the status return code
     */
    Status find_inode(const InodeId &id, InodeHeader *ino);

    /**
     * Updates an inode in the root.
     * 
     * @param id the inode ID of the inode to update
     * @param ino the new contents of the inode
     * 
     * @return the status return code
     */
    Status update_inode(const InodeId &id, const InodeHeader *ino);

    /**
     * Removes an inode from the root.
     * 
     * @param id the ID of the inode to remove
     * @param ino where to store the header of the removed inode
     * 
     * @return the status return code
     */
    Status remove_inode(const InodeId &id, InodeHeader *ino = nullptr);

    /**
     * Opens an inode by its ID.
     * 
     * @param id the ID of the inode to open
     * @param inode the inode handle to initalize with the inode
     * 
     * @return the status return code
     */
    Status open_inode(const InodeId &id, Inode &inode);

    /**
     * Opens a file by its inode ID.
     * 
     * @param id the inode ID of the file to open
     * @param file the file handle to initialize with the inode
     * 
     * @return the status return code
     */
    Status open_file(const InodeId &id, File &file);

    /**
     * Opens a directory by its inode ID.
     * 
     * @param id the inode ID of the directory to open
     * @param dir the directory handle to initialize with the inode
     * 
     * @return the status return code
     */
    Status open_directory(const InodeId &id, Directory &dir);

    /**
     * Opens an inode by its path.
     * 
     * @param path the path to the inode
     * @param inode the inode handle to initialize with the inode
     * 
     * @return the status return code
     */
    Status open_inode(const Path &path, Inode &inode);

    /**
     * Opens a file by its path.
     * 
     * @param path the path to the file
     * @param file the file handle to initialize with the inode
     * 
     * @return the status return code
     */
    Status open_file(const Path &path, File &file);

    /**
     * Opens a directory by its path.
     * 
     * @param path the path to the directory
     * @param dir the directory handle to initialize with the inode
     * 
     * @return the status return code
     */
    Status open_directory(const Path &path, Directory &dir);

    /**
     * Implicitly casts a root to its on-disk header.
     */
    operator const RootHeader &() const { return this->header; }
    operator RootHeader &() { return this->header; }
};

inline Status InoTree::on_root_change(Address new_addr) {
    *this->target = new_addr;
    return this->owner.store();
}

}
