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

template <typename K, typename V>
void bmtree<K, V>::update_root(address new_addr) {
    this->root = node<K, V>(this->fs, new_addr, this->length, this);
    this->on_root_change(new_addr);
}

template <typename K, typename V>
status bmtree<K, V>::alloc(size length, extent &target) {
    return this->alloctr(*this->fs, length, target);
}

template <typename K, typename V>
void bmtree<K, V>::free(const extent &ext) {
    this->dealloctr(*this->fs, ext);
}

template <typename K, typename V>
bmtree<K, V>::bmtree(
        brufs *fs, address addr, size length, 
        allocator all, deallocator deall,
        unsigned int max_level
    ) : 
    fs(fs), length(length), max_level(max_level), alloctr(all), dealloctr(deall),
    root(fs, addr, length, this)
{}

template <typename K, typename V>
bmtree<K, V>::bmtree(
        brufs *fs, size length, 
        allocator all, deallocator deall,
        unsigned int max_level
    ) :
    fs(fs), length(length), max_level(max_level), alloctr(all), dealloctr(deall), root(fs, this)
{}

template <typename K, typename V>
bmtree<K, V>::bmtree(const bmtree<K, V> &other) :
    fs(other.fs), length(other.length), max_level(other.max_level), alloctr(other.alloctr), 
    root(other.root)
{}

template <typename K, typename V>
bmtree<K, V> &bmtree<K, V>::operator=(const bmtree<K, V> &other) {
    assert(this->length == other.length);

    this->fs = other.fs;
    this->max_level = other.max_level;
    this->alloctr = other.alloctr;
    this->root = other.root;

    return *this;
}

template <typename K, typename V>
status bmtree<K, V>::init() {
    status status;
    extent root_extent;
    status = this->alloc(this->length, root_extent);
    if (status < 0) return status;

    node<K, V> new_root(this->fs, root_extent.offset, this->length, this);

    status = new_root.init();
    if (status < 0) {
        this->fs->free_blocks(root_extent);
        return status;
    }

    this->update_root(root_extent.offset);

    return this->root.load();
}

template <typename K, typename V>
status bmtree<K, V>::search(const K key, V &value, bool strict) {
    status status = this->root.load();
    if (status < 0) return status;

    return this->root.search(key, value, strict);
}

template <typename K, typename V>
int bmtree<K, V>::search(const K key, V *value, int max, bool strict) {
    status status = this->root.load();
    if (status < 0) return static_cast<int>(status);

    return this->root.search_all(key, value, max, strict);
}

template <typename K, typename V>
status bmtree<K, V>::insert(const K key, const V value) {
    status stt = this->root.load();
    if (stt < 0) return stt;

    return this->root.insert(key, value);
}

template <typename K, typename V>
status bmtree<K, V>::remove(const K key, V &value, bool strict) {
    status status = this->root.load();
    if (status < 0) return status;

    return this->root.remove(key, value, strict);
}

template <typename K, typename V>
status bmtree<K, V>::count_values(size &count) {
    status status = this->root.load();
    if (status < 0) return status;

    address leaf_addr;
    status = this->root.get_last_leaf(leaf_addr);
    if (status < 0) return status;

    count = 0;

    while (leaf_addr != 0) {
        node<K, V> leaf(this->fs, leaf_addr, this->length, this);
        status = leaf.load();
        if (status < 0) return status;

        count += leaf.hdr->num_values;

        leaf_addr = leaf.prev();
    }

    return status::OK;
}

template <typename K, typename V>
int bmtree<K, V>::pretty_print_root(char *buf, size len) {
    this->root.load();

    return this->root.pretty_print(buf, len, false);
}

}}
