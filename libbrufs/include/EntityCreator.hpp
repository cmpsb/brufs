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

#include "Status.hpp"
#include "InodeHeaderBuilder.hpp"
#include "Inode.hpp"
#include "File.hpp"
#include "Directory.hpp"
#include "InodeIdGenerator.hpp"

namespace Brufs {

/**
 * A high-level service capable of creating entities (bare inodes, files, directories).
 */
class EntityCreator {
private:
    const InodeIdGenerator &inode_id_generator;

public:
    /**
     * Constructs a new entity creator.
     *
     * @param gen the inode ID generator to use
     */
    EntityCreator(const InodeIdGenerator &gen) : inode_id_generator(gen) {}

    /**
     * Creates a new inode.
     *
     * @param path the path to create the inode at
     * @param ihb the inode header builder used to override default values
     * @param inode the inode to create
     *
     * @return the creation status
     */
    virtual Status create_inode(
        const Path &path, const InodeHeaderBuilder &ihb, Inode &inode
    ) const;

    /**
     * Creates a new file.
     *
     * @param path the path to create the file at
     * @param ihb the inode header builder used to override default values
     * @param file the file to create
     *
     * @return the creation status
     */
    virtual Status create_file(
        const Path &path, const InodeHeaderBuilder &ihb, File &file
    ) const;

    /**
     * Creates a new directory.
     *
     * @param path the path to create the directory at
     * @param ihb the inode header builder used to override default values
     * @param dir the directory to create
     *
     * @return the creation status
     */
    virtual Status create_directory(
        const Path &path, const InodeHeaderBuilder &ihb, Directory &dir
    ) const;
};

}
