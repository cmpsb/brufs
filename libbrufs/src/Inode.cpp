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

#include "Inode.hpp"

Brufs::Inode::Inode(Root &root) :
    root(root),
    id(0),
    header(root.create_inode_header()),
    data(reinterpret_cast<uint8_t *>(this->header) + root.get_header().inode_header_size)
{
    memset(this->header, 0, root.get_header().inode_size);
}

Brufs::Inode::Inode(Root &root, const InodeId id, const InodeHeader *header) :
    root(root),
    id(id),
    header(root.create_inode_header()),
    data(reinterpret_cast<uint8_t *>(this->header) + root.get_header().inode_header_size)
{
    memcpy(this->header, header, root.get_header().inode_size);
}

Brufs::Inode::Inode(const Inode &other) :
    root(other.root),
    id(other.id),
    header(root.create_inode_header()),
    data(reinterpret_cast<uint8_t *>(this->header) + root.get_header().inode_header_size)
{
    memcpy(this->header, other.header, root.get_header().inode_size);
}

Brufs::Inode::~Inode() {
    this->root.destroy_inode_header(this->header);
}

Brufs::Inode &Brufs::Inode::operator=(const Inode &other)  {
    this->id = other.id;
    memcpy(this->header, other.header, this->root.get_header().inode_size);

    return *this;
}

Brufs::Status Brufs::Inode::init(const InodeId id, const InodeHeader *header) {
    this->id = id;
    memcpy(this->header, header, root.get_header().inode_size);

    return Status::OK;
}

Brufs::Status Brufs::Inode::store() {
    return root.update_inode(this->id, this->header);
}
