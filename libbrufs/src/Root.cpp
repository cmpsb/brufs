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

Brufs::Status Brufs::Root::init() {
    this->it.set_value_size(this->header.inode_size);
    this->ait.set_value_size(this->header.inode_size);

    auto status = this->it.init();
    if (status < Status::OK) return status;

    return this->ait.init();
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

    if (inode.get_inode_type() != InodeType::DIRECTORY) return Status::E_WRONG_INODE_TYPE;

    directory = Directory(inode);

    return Status::OK;
}
