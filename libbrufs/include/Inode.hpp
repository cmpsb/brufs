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

#include "Brufs.hpp"
#include "InodeHeader.hpp"
#include "InodeType.hpp"
#include "RootHeader.hpp"
#include "Root.hpp"

namespace Brufs {

class Root;

class Inode {
private:
    Root &root;

    InodeId id;

    InodeHeader *header;
    uint8_t *data;

public:
    Inode(Root &root);
    Inode(Root &root, const InodeId id, const InodeHeader *header);
    Inode(const Inode &other);
    ~Inode();

    Inode &operator=(const Inode &other);

    Status init(const InodeId id, const InodeHeader *header);
    Status store();
    Status destroy() { return Status::OK; }

    InodeType get_inode_type() const {
        return static_cast<InodeType>(this->header->type);
    }

    void set_inode_type(const InodeType type) {
        this->header->type = static_cast<uint16_t>(type);
    }

    bool has_type(const InodeType type) const {
        return this->get_inode_type() == type;
    }

    InodeHeader *get_header() {
        return this->header;
    }

    const InodeHeader *get_header() const {
        return this->header;
    }

    const Root &get_root() const {
        return this->root;
    }

    Root &get_root() {
        return this->root;
    }

    uint8_t *get_data() {
        return this->data;
    }

    const uint8_t *get_data() const {
        return this->data;
    }

    Size get_data_size() const;

    operator InodeHeader *() {
        return this->header;
    }

    operator const InodeHeader *() const {
        return this->header;
    }
};

}
