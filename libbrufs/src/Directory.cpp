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

#include "Directory.hpp"

namespace Brufs { namespace BmTree {

template <>
bool equiv_values(const DirectoryEntry *current, const DirectoryEntry *replacement) {
    return strncmp(current->label, replacement->label, MAX_LABEL_LENGTH) == 0;
}

}}

Brufs::Status Brufs::Directory::init(const InodeId &id, const InodeHeader *hdr) {
    Inode::init(id, hdr);

    FileEntryTree entries(*this);
    auto status = entries.init();
    if (status < Status::OK) return status;

    return this->store();
}

Brufs::Status Brufs::Directory::destroy() {
    auto status = Inode::destroy();
    if (status < Status::OK) return status;

    FileEntryTree entries(*this);
    return entries.destroy();
}

Brufs::Status Brufs::Directory::look_up(const char *name, DirectoryEntry &target) {
    assert(name);

    FileEntryTree entries(*this);

    const auto hash = XXH64(name, strnlen(name, MAX_LABEL_LENGTH), HASH_SEED);

    DirectoryEntry candidates[MAX_COLLISIONS];
    int num = entries.search(hash, candidates, MAX_COLLISIONS, true);
    if (num < 0) return static_cast<Status>(num);

    for (int i = 0; i < num; ++i) {
        if (strncmp(name, candidates[i].label, MAX_LABEL_LENGTH) != 0) continue;

        target = candidates[i];

        return Status::OK;
    }

    return Status::E_NOT_FOUND;
}

Brufs::Status Brufs::Directory::insert(const DirectoryEntry &entry) {
    DirectoryEntry dummy;
    Status status = this->look_up(entry.label, dummy);
    if (status == Status::OK) return Status::E_EXISTS;
    if (status != Status::E_NOT_FOUND) return status;

    FileEntryTree entries(*this);
    return entries.insert(entry.hash(), entry);
}

Brufs::Status Brufs::Directory::update(const DirectoryEntry &entry) {
    FileEntryTree entries(*this);

    return entries.update(entry.hash(), entry);
}

Brufs::Status Brufs::Directory::remove(const DirectoryEntry &entry) {
    FileEntryTree entries(*this);

    DirectoryEntry dummy;
    return entries.remove(entry.hash(), dummy, true);
}

Brufs::Status Brufs::Directory::remove(const char *name, DirectoryEntry &entry) {
    FileEntryTree entries(*this);

    entry.set_label(name);

    return entries.remove(entry.hash(), entry, true);
}

Brufs::Status Brufs::Directory::remove(const char *name) {
    FileEntryTree entries(*this);

    DirectoryEntry lbl_entry;
    lbl_entry.set_label(name);

    return entries.remove(lbl_entry.hash(), lbl_entry);
}

Brufs::SSize Brufs::Directory::count() {
    Size count;
    FileEntryTree entries(*this);
    auto status = entries.count_values(count);

    if (status < Status::OK) return static_cast<SSize>(status);

    return static_cast<SSize>(count);
}

Brufs::Status Brufs::Directory::collect(Vector<DirectoryEntry> &entries) {
    entries.clear();
    entries.reserve(this->count());

    FileEntryTree tree(*this);
    return tree.walk<Vector<DirectoryEntry> *>([](auto e, auto v) {
        v->push_back(e);
        return Status::OK;
    }, &entries);
}
