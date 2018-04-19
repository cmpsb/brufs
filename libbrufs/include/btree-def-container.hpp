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

#include "rtstructures.hpp"

#include <cstdio>
#include <ctime>

namespace brufs { namespace bmtree {

template <typename K, typename V, allocator A>
void bmtree<K, V, A>::update_root(address new_addr) {
    this->root = node(this->fs, new_addr, this->length, this);
    this->on_root_change(new_addr);
}

template <typename K, typename V, allocator A>
bmtree<K, V, A>::bmtree(brufs &fs, address addr, size length, unsigned int max_level) : 
    fs(fs), length(length), max_level(max_level), root(fs, addr, length, this) 
{}

template <typename K, typename V, allocator A>
bmtree<K, V, A>::bmtree(brufs &fs, size length, unsigned int max_level) :
    fs(fs), length(length), max_level(max_level), root(fs, this)
{}

template <typename K, typename V, allocator A>
bmtree<K, V, A>::bmtree(const bmtree<K, V, A> &other) :
    fs(other.fs), length(other.length), max_level(other.max_level), root(other.root)
{}

template <typename K, typename V, allocator A>
bmtree<K, V, A> &bmtree<K, V, A>::operator=(const bmtree<K, V, A> &other) {
    assert(this->length == other.length);

    this->fs = other.fs;
    this->max_level = other.max_level;
    this->root = other.root;

    return *this;
}

template <typename K, typename V, allocator A>
status bmtree<K, V, A>::init() {
    status status;
    extent root_extent;
    status = A(this->fs, this->length, root_extent);
    if (status < 0) return status;

    node<K, V, A> new_root(this->fs, root_extent.offset, this->length, this);
    memset(new_root.buf, 0, this->length);
    new (new_root.hdr) header;

    status = new_root.store();
    if (status < 0) {
        this->fs.free_blocks(root_extent);
        return status;
    }

    this->update_root(root_extent.offset);

    return this->root.load();
}

template <typename K, typename V, allocator A>
status bmtree<K, V, A>::search(const K key, V &value, bool strict) {
    status status = this->root.load();
    if (status < 0) return status;

    return this->root.search(key, value, strict);
}

template <typename K, typename V, allocator A>
status bmtree<K, V, A>::insert(const K key, const V value) {
    status stt = this->root.load();
    if (stt < 0) return stt;

    return this->root.insert(key, value);
}

template <typename K, typename V, allocator A>
status bmtree<K, V, A>::remove(const K key, V &value, bool strict) {
    status status = this->root.load();
    if (status < 0) return status;

    return this->root.remove(key, value, strict);
}

template <typename K, typename V, allocator A>
status bmtree<K, V, A>::count_values(size &count) {
    status status = this->root.load();
    if (status < 0) return status;

    address leaf_addr;
    status = this->root.get_last_leaf(leaf_addr);
    if (status < 0) return status;

    count = 0;

    while (leaf_addr != 0) {
        node<K, V, A> leaf(this->fs, leaf_addr, this->length, this);
        status = leaf.load();
        if (status < 0) return status;

        count += leaf.hdr->num_values;

        leaf_addr = leaf.prev();
    }

    return status::OK;
}

template <typename K, typename V, allocator A>
int bmtree<K, V, A>::pretty_print_root(char *buf, size len) {
    this->root.load();

    return this->root.pretty_print(buf, len, false);
}

}}
