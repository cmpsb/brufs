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

#include "types.hpp"
#include "Brufs.hpp"
#include "Root.hpp"
#include "RootHeader.hpp"
#include "Inode.hpp"
#include "File.hpp"
#include "Directory.hpp"

static bool is_main_stream(const Brufs::InodeId &id) {
    return (id & 0b111111) == 0;
}

Brufs::Status Brufs::Root::store() {
    if (!this->enable_store) return Status::OK;
    return this->fs.update_root(this->header);
}

Brufs::Root::Root(Brufs &fs, const RootHeader &hdr) :
    fs(fs), header(hdr),
    it(&fs, *this, &this->header.int_address, fs.get_header().cluster_size),
    ait(&fs, *this, &this->header.ait_address, fs.get_header().cluster_size)
{
    this->it.set_value_size(this->header.inode_size);
    this->ait.set_value_size(this->header.inode_size);
}

Brufs::Status Brufs::Root::init(const InodeHeaderBuilder &ihb) {
    this->enable_store = false;

    // Initialize the core datastructures
    this->it.set_value_size(this->header.inode_size);
    this->ait.set_value_size(this->header.inode_size);

    auto status = this->it.init();
    if (status < Status::OK) return status;

    status = this->ait.init();
    if (status < Status::OK) return status;

    // Create a root directory
    InodeHeader drdh;
    drdh.created = drdh.last_modified = Timestamp::now();
    drdh.owner = drdh.group = 0;
    drdh.num_links = 1;
    drdh.type = InodeType::DIRECTORY;
    drdh.flags = 0;
    drdh.mode = 0755;
    drdh.file_size = 0;
    drdh.checksum = 0;

    Directory root_dir(*this);

    auto rdh = ihb.build(drdh);
    memcpy(root_dir.get_header(), &rdh, sizeof(InodeHeader));

    status = root_dir.init(ROOT_DIR_INODE_ID, root_dir.get_header());
    if (status < Status::OK) return status;

    status = this->insert_inode(ROOT_DIR_INODE_ID, root_dir.get_header());
    if (status < Status::OK) return status;

    status = root_dir.insert(".", ROOT_DIR_INODE_ID);
    if (status < Status::OK) return status;

    status = root_dir.insert("..", ROOT_DIR_INODE_ID);
    if (status < Status::OK) return status;

    this->enable_store = true;
    return Status::OK;
}

Brufs::InodeHeader *Brufs::Root::create_inode_header() const {
    auto buf = new char[this->header.inode_size];
    return new (buf) InodeHeader;
}

void Brufs::Root::destroy_inode_header(InodeHeader *header) const {
    delete[] header;
}

Brufs::Status Brufs::Root::insert_inode(const InodeId &id, const InodeHeader *ino) {
    if (is_main_stream(id)) return this->it.insert(id, ino, true);

    return this->ait.insert(id, ino, true);
}

Brufs::Status Brufs::Root::find_inode(const InodeId &id, InodeHeader *ino) {
    if (is_main_stream(id)) return this->it.search(id, ino, true);

    return this->ait.search(id, ino, true);
}

Brufs::Status Brufs::Root::update_inode(const InodeId &id, const InodeHeader *ino) {
    if (is_main_stream(id)) return this->it.update(id, ino);

    return this->ait.update(id, ino);
}

Brufs::Status Brufs::Root::remove_inode(const InodeId &id, InodeHeader *ino) {
    if (is_main_stream(id)) return this->it.remove(id, ino, true);

    return this->ait.insert(id, ino, true);
}

Brufs::Status Brufs::Root::open_inode(const InodeId &id, Inode &inode) {
    auto header = this->create_inode_header();

    auto status = this->find_inode(id, header);
    if (status < Status::OK) return status;

    inode = Inode(*this, id, header);

    this->destroy_inode_header(header);

    return Status::OK;
}

Brufs::Status Brufs::Root::open_file(const InodeId &id, File &file) {
    Inode inode(*this);

    auto status = this->open_inode(id, inode);
    if (status < Status::OK) return status;

    if (inode.get_inode_type() != InodeType::FILE) return Status::E_WRONG_INODE_TYPE;

    file = File(inode);

    return Status::OK;
}

Brufs::Status Brufs::Root::open_directory(const InodeId &id, Directory &directory) {
    Inode inode(*this);

    auto status = this->open_inode(id, inode);
    if (status < Status::OK) return status;

    if (inode.get_inode_type() != InodeType::DIRECTORY) return Status::E_NOT_DIR;

    directory = Directory(inode);

    return Status::OK;
}

Brufs::Status Brufs::Root::open_inode(const Path &path, Inode &inode) {
    if (path.get_components().get_size() == 0) {
        return this->open_inode(ROOT_DIR_INODE_ID, inode);
    }

    Directory dir(*this);
    auto status = this->open_directory(path.get_parent(), dir);
    if (status == Status::E_WRONG_INODE_TYPE || status == Status::E_NOT_DIR) {
        return Status::E_NOT_DIR;
    }

    DirectoryEntry dir_entry;
    status = dir.look_up(path.get_components().back().c_str(), dir_entry);
    if (status < Status::OK) return status;

    return this->open_inode(dir_entry.inode_id, inode);
}

Brufs::Status Brufs::Root::open_file(const Path &path, File &file) {
    auto status = this->open_inode(path, file);
    if (status < Status::OK) return status;

    if (file.get_inode_type() != InodeType::FILE) return Status::E_WRONG_INODE_TYPE;
    return Status::OK;
}

Brufs::Status Brufs::Root::open_directory(const Path &path, Directory &dir) {
    auto status = this->open_directory(ROOT_DIR_INODE_ID, dir);
    if (status < Status::OK) return status;

    for (const auto &component : path.get_components()) {
        DirectoryEntry dir_entry;
        status = dir.look_up(component.c_str(), dir_entry);
        if (status < Status::OK) return status;

        status = this->open_directory(dir_entry.inode_id, dir);
        if (status < Status::OK) return status;
    }

    return Status::OK;
}
