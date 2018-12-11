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

#include "EntityCreator.hpp"

Brufs::Status Brufs::EntityCreator::create_inode(
    const Path &path, const InodeHeaderBuilder &ihb, Inode &inode
) const {
    auto &root = inode.get_root();

    Directory parent(root);
    auto status = root.open_directory(path.get_parent(), parent);
    if (status < Status::OK) return status;

    InodeHeader defaults;
    defaults.created = defaults.last_modified = Timestamp::now();
    defaults.owner = parent.get_header()->owner;
    defaults.group = parent.get_header()->group;
    defaults.num_links = 1;
    defaults.type = InodeType::NONE;
    defaults.flags = 0;
    defaults.mode = parent.get_header()->mode & ~0111;
    defaults.file_size = 0;

    const auto header = ihb.build(defaults);
    auto id = this->inode_id_generator.generate();

    auto hdr = root.create_inode_header();
    memcpy(hdr, &header, sizeof(InodeHeader));

    status = inode.init(id, hdr);

    root.destroy_inode_header(hdr);
    if (status < Status::OK) return status;

    status = root.insert_inode(id, inode);
    if (status < Status::OK) return status;

    DirectoryEntry entry;
    entry.inode_id = id;
    entry.set_label(path.get_components().back());

    return parent.insert(entry);
}

Brufs::Status Brufs::EntityCreator::create_file(
    const Path &path, const InodeHeaderBuilder &ihb, File &file
) const {
    InodeHeaderBuilder ihb_copy = ihb;
    ihb_copy.with_type(InodeType::FILE);

    return this->create_inode(path, ihb_copy, file);
}

Brufs::Status Brufs::EntityCreator::create_directory(
    const Path &path, const InodeHeaderBuilder &ihb, Directory &dir
) const {
    if (path.get_components().empty()) return Status::OK;

    auto &root = dir.get_root();

    Directory parent(root);
    auto status = root.open_directory(path.get_parent(), parent);
    if (status < Status::OK) return status;

    InodeHeader defaults;
    defaults.created = defaults.last_modified = Timestamp::now();
    defaults.owner = parent.get_header()->owner;
    defaults.group = parent.get_header()->group;
    defaults.num_links = 1;
    defaults.type = InodeType::DIRECTORY;
    defaults.flags = 0;
    defaults.mode = parent.get_header()->mode;
    defaults.file_size = 0;

    const auto header = ihb.build(defaults);
    auto id = this->inode_id_generator.generate();

    auto hdr = root.create_inode_header();
    memcpy(hdr, &header, sizeof(InodeHeader));

    status = dir.init(id, hdr);
    if (status < Status::OK) return status;

    root.destroy_inode_header(hdr);
    if (status < Status::OK) return status;

    status = root.insert_inode(id, dir);
    if (status < Status::OK) return status;

    status = parent.insert(path.get_components().back(), id);
    if (status < Status::OK) return status;

    status = dir.insert(".", id);
    if (status < Status::OK) return status;

    return dir.insert("..", parent.get_id());
}
