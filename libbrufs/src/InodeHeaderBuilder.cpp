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

#include "InodeHeaderBuilder.hpp"

Brufs::InodeHeader Brufs::InodeHeaderBuilder::build(const Brufs::InodeHeader &def) const {
    Brufs::InodeHeader ret;

    ret.created = this->created_set ? this->proto.created : def.created;
    ret.last_modified = this->last_modified_set ? this->proto.last_modified : def.last_modified;
    ret.owner = this->owner_set ? this->proto.owner : def.owner;
    ret.group = this->group_set ? this->proto.group : def.group;
    ret.num_links = this->num_links_set ? this->proto.num_links : def.num_links;
    ret.type = this->type_set ? this->proto.type : def.type;
    ret.flags = this->flags_set ? this->proto.flags : def.flags;
    ret.mode = this->mode_set ? this->proto.mode : def.mode;
    ret.file_size = this->file_size_set ? this->proto.file_size : def.file_size;

    return ret;
}
