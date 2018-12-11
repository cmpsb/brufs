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

#include "InodeHeader.hpp"

namespace Brufs {

class InodeHeaderBuilder {
private:
    InodeHeader proto;

    bool created_set = false;
    bool last_modified_set = false;
    bool owner_set = false;
    bool group_set = false;
    bool num_links_set = false;
    bool type_set = false;
    bool flags_set = false;
    bool mode_set = false;
    bool file_size_set = false;

public:
    InodeHeaderBuilder &with_created(const Timestamp &created) {
        proto.created = created;
        this->created_set = true;
        return *this;
    }

    InodeHeaderBuilder &with_last_modified(const Timestamp &last_modified) {
        proto.last_modified = last_modified;
        this->last_modified_set = true;
        return *this;
    }

    InodeHeaderBuilder &with_owner(const OwnerId &owner) {
        proto.owner = owner;
        this->owner_set = true;
        return *this;
    }

    InodeHeaderBuilder &with_group(const OwnerId &group) {
        proto.group = group;
        this->group_set = true;
        return *this;
    }

    InodeHeaderBuilder &with_num_links(const uint16_t &num_links) {
        proto.num_links = num_links;
        this->num_links_set = true;
        return *this;
    }

    InodeHeaderBuilder &with_type(const uint16_t &type) {
        proto.type = type;
        this->type_set = true;
        return *this;
    }

    InodeHeaderBuilder &with_flags(const uint16_t &flags) {
        proto.flags = flags;
        this->flags_set = true;
        return *this;
    }

    InodeHeaderBuilder &with_mode(const uint16_t &mode) {
        proto.mode = mode;
        this->mode_set = true;
        return *this;
    }

    InodeHeaderBuilder &with_created(const Size &file_size) {
        proto.file_size = file_size;
        this->file_size_set = true;
        return *this;
    }

    InodeHeader build(const InodeHeader &defaults) const;
};

}
