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

#include "types.hpp"

#include <cstdio>
#include <ctime>

namespace Brufs { namespace BmTree {

template <typename K, typename V>
Status BmTree<K, V>::update_root(Address new_addr, Size length) {
    if (length != 0) this->length = length;
    this->root = Node<K, V>(this->fs, new_addr, this->length, this);
    return this->on_root_change(new_addr);
}

template <typename K, typename V>
Status BmTree<K, V>::alloc(Size length, Extent &target) {
    return this->alloctr(*this->fs, length, target);
}

template <typename K, typename V>
Status BmTree<K, V>::free(const Extent &ext) {
    return this->dealloctr(*this->fs, ext);
}

template <typename K, typename V>
BmTree<K, V>::BmTree(
        Brufs *fs, Address addr, Size length,
        Allocator all, Deallocator deall,
        unsigned int max_level
    ) :
    fs(fs), length(length), max_level(max_level), value_size(sizeof(V)),
    alloctr(all), dealloctr(deall), root(fs, addr, length, this)
{}

template <typename K, typename V>
BmTree<K, V>::BmTree(
        Brufs *fs, Size length,
        Allocator all, Deallocator deall,
        unsigned int max_level
    ) :
    fs(fs), length(length), max_level(max_level), value_size(sizeof(V)),
    alloctr(all), dealloctr(deall), root(fs, this)
{}

template <typename K, typename V>
BmTree<K, V>::BmTree(const BmTree<K, V> &other) :
    fs(other.fs), length(other.length), max_level(other.max_level), alloctr(other.alloctr),
    root(other.root)
{}

template <typename K, typename V>
BmTree<K, V> &BmTree<K, V>::operator=(const BmTree<K, V> &other) {
    assert(this->length == other.length);

    this->fs = other.fs;
    this->max_level = other.max_level;
    this->alloctr = other.alloctr;
    this->root = other.root;

    return *this;
}

template <typename K, typename V>
Status BmTree<K, V>::init(Size length) {
    if (length != 0) {
        this->length = length;
    }

    Status status;
    Extent root_extent;
    status = this->alloc(this->length, root_extent);
    if (status < 0) return status;

    Node<K, V> new_root(this->fs, root_extent.offset, this->length, this);

    status = new_root.init();
    if (status < 0) {
        this->fs->free_blocks(root_extent);
        return status;
    }

    return this->update_root(root_extent.offset);
}

template <typename K, typename V>
Status BmTree<K, V>::search(const K key, V *value, bool strict) {
    auto status = this->root.load();
    if (status < 0) return status;

    return this->root.search(key, value, strict);
}

template <typename K, typename V>
template <typename P>
int BmTree<K, V>::search(const K key, P *value, int max, bool strict) {
    Status status = this->root.load();
    if (status < 0) return static_cast<int>(status);

    return this->root.search_all(key, reinterpret_cast<uint8_t *>(value), max, strict);
}

template <typename K, typename V>
Status BmTree<K, V>::get_first(V *value) {
    auto status = this->root.load();
    if (status < Status::OK) return status;

    return this->root.get_first(value);
}

template <typename K, typename V>
Status BmTree<K, V>::get_last(V *value) {
    auto status = this->root.load();
    if (status < Status::OK) return status;

    return this->root.get_last(value);
}

template <typename K, typename V>
Status BmTree<K, V>::insert(const K key, const V *value, bool collide) {
    Status stt = this->root.load();
    if (stt < 0) return stt;

    return this->root.insert(key, value, collide);
}

template <typename K, typename V>
Status BmTree<K, V>::update(const K key, const V *value) {
    Status stt = this->root.load();
    if (stt < 0) return stt;

    return this->root.update(key, value);
}

template <typename K, typename V>
Status BmTree<K, V>::remove(const K key, V *value, bool strict) {
    Status status = this->root.load();
    if (status < 0) return status;

    return this->root.remove(key, value, strict);
}

template <typename K, typename V>
Status BmTree<K, V>::count_values(Size &count) {
    Status status = this->root.load();
    if (status < 0) return status;

    Address leaf_addr;
    status = this->root.get_last_leaf(leaf_addr);
    if (status < 0) return status;

    count = 0;

    while (leaf_addr != 0) {
        Node<K, V> leaf(this->fs, leaf_addr, this->length, this);
        status = leaf.load();
        if (status < 0) return status;

        count += leaf.hdr->num_values;

        assert(leaf_addr != leaf.prev());
        leaf_addr = leaf.prev();
    }

    return Status::OK;
}

template <typename K, typename V>
Status BmTree<K, V>::count_used_space(Size &size) {
    auto status = this->root.load();
    if (status < Status::OK) return status;

    return this->root.count_used_space(size);
}

template <typename K, typename V>
template <typename P>
Status BmTree<K, V>::destroy(EntryConsumer<V, P> destroyer, P payload) {
    auto status = this->root.load();
    if (status < Status::OK) return status;

    return this->root.destroy(destroyer, payload);
}

template <typename K, typename V>
Status BmTree<K, V>::destroy(ContextlessEntryConsumer<V> destroyer) {
    return this->destroy<ContextlessEntryConsumer<V>>([](auto v, auto p) {
        return p(v);
    }, destroyer);
}

template <typename K, typename V>
Status BmTree<K, V>::destroy() {
    return this->destroy([](UNUSED auto v) { return Status::OK; });
}

template <typename K, typename V>
template <typename P>
Status BmTree<K, V>::walk(EntryConsumer<V, P> consumer, P payload) {
    Status stt = this->root.load();
    if (stt < Status::OK) return stt;

    Address leaf_addr;
    stt = this->root.get_last_leaf(leaf_addr);
    if (stt < Status::OK) return stt;

    while (leaf_addr != 0) {
        Node<K, V> leaf(this->fs, leaf_addr, this->length, this);
        stt = leaf.load();
        if (stt < Status::OK) return stt;

        auto values = leaf.template get_values<V>();

        for (SSize i = static_cast<Size>(leaf.hdr->num_values) - 1; i >= 0; --i) {
            do stt = consumer(values[i], payload);
            while (stt == Status::RETRY);

            if (stt == Status::STOP) return Status::OK;
            if (stt < Status::OK) return stt;
        }

        leaf_addr = leaf.prev();
    }

    return Status::OK;
}

template <typename K, typename V>
Status BmTree<K, V>::walk(ContextlessEntryConsumer<V> consumer) {
    return this->walk<ContextlessEntryConsumer<V>>([](auto v, auto p) { return p(v); }, consumer);
}

template <typename K, typename V>
int BmTree<K, V>::pretty_print_root(char *buf, Size len) {
    this->root.load();

    return this->root.pretty_print(buf, len, false);
}

}}
