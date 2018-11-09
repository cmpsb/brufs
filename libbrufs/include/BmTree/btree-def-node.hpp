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

#include <cstdio>

#include "../types.hpp"
#include "../io.hpp"
#include "../Brufs.hpp"

namespace Brufs::BmTree {

template<typename K, typename V>
Node<K, V>::Node(Brufs *fs, Address addr, Size length, BmTree<K, V> *container,
                 Node<K, V> *parent, unsigned int index_in_parent) :
    fs(fs), addr(addr), length(length), container(container), parent(parent),
    index_in_parent(index_in_parent)
{
    this->buf = static_cast<char *>(malloc(length));
    this->hdr = reinterpret_cast<Header *>(this->buf);
}

template<typename K, typename V>
Node<K, V>::Node(Brufs *fs, BmTree<K, V> *container) :
    fs(fs), addr(0), length(0), container(container), parent(nullptr), index_in_parent(UINT_MAX)
{
    this->buf = NULL;
    this->hdr = NULL;
}

template<typename K, typename V>
Node<K, V>::Node(const Node<K, V> &other) :
    Node(other.fs, other.addr, other.length, other.container, other.parent)
{}

template <typename K, typename V>
Node<K, V>::~Node() {
    free(this->buf);
}

template<typename K, typename V>
Node<K, V> &Node<K, V>::operator=(const Node<K, V> &other) {
    if (this->length != other.length || !this->buf) {
        this->~Node();
        this->buf = static_cast<char *>(malloc(other.length));
        this->hdr = reinterpret_cast<Header *>(this->buf);
    }

    this->fs = other.fs;
    this->addr = other.addr;
    this->length = other.length;
    this->container = other.container;
    this->parent = other.parent;

    return *this;
}

template <typename K, typename V>
Status Node<K, V>::init() {
    memset(this->buf, 0, this->length);
    new (this->hdr) Header;
    this->hdr->size = next_multiple_of(this->hdr->size, alignof(K));
    this->hdr->size = this->hdr->size >= sizeof(K) ? this->hdr->size : sizeof(K);

    return this->store();
}

template <typename K, typename V>
auto Node<K, V>::get_record_size() {
    return (this->hdr->level > 0) ? sizeof(Address) : this->container->get_value_size();
}

template<typename K, typename V>
template <typename R>
auto Node<K, V>::get_cap() {
    auto usable = (this->length - this->hdr->size - sizeof(Address));
    return usable / next_multiple_of(sizeof(K) + this->get_record_size(), alignof(R));
}

template<typename K, typename V>
template<typename R>
R *Node<K, V>::get_values() {
    return reinterpret_cast<R *>(
        this->buf + next_multiple_of(this->hdr->size + this->get_cap<R>() * sizeof(K), alignof(R))
    );
}

template <typename K, typename V>
template <typename R>
R *Node<K, V>::get_value(unsigned int idx) {
    return reinterpret_cast<R *>(
        this->buf + next_multiple_of(this->hdr->size + this->get_cap<R>() * sizeof(K), alignof(R))
        + idx * this->get_record_size()
    );
}

template<typename K, typename V>
K *Node<K, V>::get_keys() {
    return reinterpret_cast<K *>(this->buf + this->hdr->size);
}

template<typename K, typename V>
Address *Node<K, V>::get_link() {
    return reinterpret_cast<Address *>(this->buf + this->length - sizeof(Address));
}

template <typename K, typename V>
Address &Node<K, V>::prev() {
    return *this->get_link();
}

template<typename K, typename V>
Status Node<K, V>::load() {
    assert(this->buf);

    SSize status = dread(this->fs->get_disk(), this->buf, this->length, this->addr);
    if (status < 0) return static_cast<::Brufs::Status>(status);

    if (memcmp(this->hdr->magic, "B+", 2) != 0) return Status::E_BAD_MAGIC;
    if (this->hdr->size % 8 > 0) return Status::E_MISALIGNED;

    return Status::OK;
}

template<typename K, typename V>
Status Node<K, V>::store() {

    SSize status = dwrite(this->fs->get_disk(), this->buf, this->length, this->addr);
    if (status < 0) return static_cast<::Brufs::Status>(status);

    return Status::OK;
}

template<typename K, typename V>
void Node<K, V>::locate(const K &key, unsigned int &result) {
    assert(this->hdr->num_values > 0);

    auto keys = this->get_keys();

    unsigned int i = 0;
    for (; i < (this->hdr->num_values - 1) && key >= keys[i]; ++i);

    result = i;
}

template<typename K, typename V>
Status Node<K, V>::locate_in_leaf(const K &key, unsigned int &result) {
    assert(this->hdr->level == 0);
    assert(this->hdr->num_values > 0);

    auto keys = this->get_keys();

    unsigned int i = 0;
    for (; i < this->hdr->num_values && key > keys[i]; ++i);
    while (key == keys[i + 1] && i < this->hdr->num_values - 1) ++i;

    result = i;
    if (i >= this->hdr->num_values) return Status::E_NOT_FOUND;

    return Status::OK;
}

template<typename K, typename V>
Status Node<K, V>::locate_in_leaf_strict(const K &key, unsigned int &result) {
    assert(this->hdr->level == 0);
    assert(this->hdr->num_values > 0);

    auto keys = this->get_keys();

    unsigned int i = 0;
    for (; i < this->hdr->num_values && key != keys[i]; ++i);

    result = i;
    if (i >= this->hdr->num_values) return Status::E_NOT_FOUND;

    return Status::OK;
}

template <typename K, typename V>
Status Node<K, V>::search(const K &key, V *value, bool strict) {
    int status = this->search_all(key, reinterpret_cast<uint8_t *>(value), 1, strict);
    if (status < 0) return static_cast<::Brufs::Status>(status);
    if (status == 0) return Status::E_NOT_FOUND;
    return ::Brufs::Status::OK;
}

template<typename K, typename V>
int Node<K, V>::search_all(const K &key, uint8_t *value, int max, bool strict) {
    assert(max >= 0);
    if (max == 0) return 0;

    if (this->hdr->num_values == 0) return Status::E_NOT_FOUND;
    if (this->hdr->num_values == 1) {
        assert(this->hdr->level == 0);
        auto keys = this->get_keys();
        if (!strict && key > keys[0]) return Status::E_NOT_FOUND;
        if (strict && key != keys[0]) return Status::E_NOT_FOUND;

        memcpy(value, this->get_value<V>(0), this->get_record_size());

        return 1;
    }

    if (this->hdr->level > 0) {
        // This is an inner node, look up the address of the next node
        auto values = this->get_values<Address>();

        unsigned int idx;
        this->locate(key, idx);

        Node<K, V> subtree(
            this->fs, values[idx], this->length, this->container, this, idx
        );

        Status status = subtree.load();
        if (status < 0) return status;

        return subtree.search_all(key, value, max, strict);
    }

    unsigned int idx;
    Status stt = this->locate_in_leaf(key, idx);
    if (stt < 0) return stt;

    return this->copy_while(key, value, idx, max, strict);
}

template <typename K, typename V>
int Node<K, V>::copy_while(const K &key, uint8_t *value, unsigned int start, int max, bool strict) {
    assert(max >= 0);
    if (max == 0) return 0;

    auto keys = this->get_keys();

    Size i = start;
    int k = 0;
    while (!strict || keys[i] == key) {
        memcpy(
            value + k * this->get_record_size(),
            this->get_value<V>(i),
            this->get_record_size()
        );

        ++k;

        if (k >= max) return k;
        if (i == 0) break;

        --i;
    }

    // if (k == 0) return 0;
    if (this->prev() == 0) return k;

    Node<K, V> pred(
        this->fs, this->prev(), this->length, this->container, this, this->index_in_parent - 1
    );
    Status status = pred.load();
    if (status < 0) return status;

    int stt = pred.copy_while(
        key, value + k * this->get_record_size(), pred.hdr->num_values - 1, max - k, strict
    );
    if (stt < 0) return stt;

    return stt + k;
}

template <typename K, typename V>
Status Node<K, V>::get_first(V *value) {
    if (this->hdr->num_values == 0) return Status::E_NOT_FOUND;
    if (this->hdr->level > 0) {
        // This is an inner node, look up the address of the next node
        auto values = this->get_values<Address>();

        Node<K, V> subtree(
            this->fs, values[0], this->length, this->container, this, 0
        );

        Status status = subtree.load();
        if (status < 0) return status;

        return subtree.get_first(value);
    }

    memcpy(value, this->get_value<V>(0), this->get_record_size());

    return Status::OK;
}

template <typename K, typename V>
Status Node<K, V>::get_last(V *value) {
    if (this->hdr->num_values == 0) return Status::E_NOT_FOUND;

    const auto idx = this->hdr->num_values - 1;

    if (this->hdr->level > 0) {
        // This is an inner node, look up the address of the next node
        auto values = this->get_values<Address>();

        Node<K, V> subtree(
            this->fs, values[idx], this->length, this->container, this, idx
        );

        Status status = subtree.load();
        if (status < 0) return status;

        return subtree.get_last(value);
    }

    memcpy(value, this->get_value<V>(idx), this->get_record_size());

    return Status::OK;
}

template<typename K, typename V>
Status Node<K, V>::insert_initial(const K &key, Address left, Address right) {
    this->hdr->num_values = 2;

    auto keys = this->get_keys();
    auto values = this->get_values<Address>();

    keys[0] = key;
    values[0] = left;
    values[1] = right;

    this->prev() = 0;

    return this->store();
}

template <typename K, typename V>
template <typename R>
Status Node<K, V>::split(const K &key, const R *value, const unsigned int idx) {
    Status status;

    auto keys = this->get_keys();
    auto values = this->get_values<R>();

    auto num_left = this->hdr->num_values / 2;
    auto num_right = this->hdr->num_values - num_left;

    this->hdr->num_values = num_right;

    Extent sibling_extent;
    status = this->container->alloc(this->length, sibling_extent);
    if (status < 0) return status;

    Node<K, V> sibling(
        this->fs, sibling_extent.offset, this->length, this->container, this
    );
    memset(sibling.buf, 0, this->length);
    new (sibling.hdr) Header(this->hdr->level, num_left);
    sibling.hdr->size = this->hdr->size;

    auto sibling_keys = sibling.get_keys();
    auto sibling_values = sibling.get_values<R>();

    memcpy(sibling_keys, keys, num_left * sizeof(K));
    memcpy(sibling_values, values, num_left * this->get_record_size());

    sibling.prev() = this->prev();
    this->prev() = sibling_extent.offset;

    status = sibling.store();
    if (status < 0) {
        (void) this->container->free(sibling_extent);
        return status;
    }

    memmove(keys, keys + num_left, num_right * sizeof(K));
    memmove(values, this->get_value<R>(num_left), num_right * this->get_record_size());

    status = this->store();
    if (status < 0) {
        (void) this->container->free(sibling_extent);
        return status;
    }

    if (this->parent != nullptr) {
        status = this->parent->insert_direct_at<Address>(
            sibling_keys[num_left - 1], &sibling_extent.offset, this->index_in_parent
        );
        if (status < 0) {
            (void) this->container->free(sibling_extent);
            return status;
        }

        ++this->index_in_parent;

        if (idx <= num_left) {
            return sibling.insert_direct_at<R>(key, value, idx);
        }

        return this->insert_direct_at<R>(key, value, idx - num_left);
    } else {
        Extent parent_extent;
        status = this->container->alloc(this->length, parent_extent);
        if (status < 0) {
            (void) this->container->free(sibling_extent);
            return status;
        }

        // Create a new parent
        Node<K, V> pstor(
            this->fs, parent_extent.offset, this->length, this->container
        );
        this->parent = &pstor;

        memset(this->parent->buf, 0, this->length);
        new (this->parent->hdr) Header(this->hdr->level + 1);
        this->parent->hdr->size = this->hdr->size;

        status = this->parent->insert_initial(
            sibling_keys[num_left - 1], sibling_extent.offset, this->addr
        );
        if (status < 0) return status;

        if (idx <= num_left) {
            status = sibling.insert_direct_at<R>(key, value, idx);
        } else {
            status = this->insert_direct_at<R>(key, value, idx - num_left);
        }
        if (status < 0) return status;

        return this->container->update_root(parent_extent.offset);
    }
}

template<typename K, typename V>
template <typename R>
Status Node<K, V>::insert_direct(const K &key, const R *value, bool collide) {
    unsigned int idx;
    if (this->hdr->level > 0) this->locate(key, idx);
    else (void) this->locate_in_leaf(key, idx);

    return this->insert_direct_at(key, value, idx, collide);
}

template <typename K, typename V>
template <typename R>
Status Node<K, V>::insert_direct_at(const K &key, const R *value, unsigned int idx, bool collide) {
    auto key_cap = this->get_cap<R>();
    if (this->hdr->num_values >= key_cap) return this->split<R>(key, value, idx);

    auto keys = this->get_keys();

    if (collide) {
        if (this->hdr->level > 0 && idx < (this->hdr->num_values - 1) && keys[idx] == key) {
            return Status::E_EXISTS;
        } else if (this->hdr->level == 0 && keys[idx] == key) {
            return Status::E_EXISTS;
        }
    }

    memmove(keys + idx + 1, keys + idx, (this->hdr->num_values - idx) * sizeof(K));
    keys[idx] = key;

    memmove(
        this->get_value<R>(idx + 1),
        this->get_value<R>(idx),
        (this->hdr->num_values - idx) * this->get_record_size()
    );
    memcpy(this->get_value<R>(idx), value, this->get_record_size());

    ++this->hdr->num_values;

    return this->store();
}

template<typename K, typename V>
Status Node<K, V>::insert(const K &key, const V *value, bool collide) {
    if (this->hdr->num_values == 0) {
        assert(this->hdr->level == 0);

        K *keys = this->get_keys();

        keys[0] = key;
        memcpy(this->get_value<V>(0), value, this->get_record_size());

        this->hdr->num_values = 1;
        return this->store();
    }

    if (this->hdr->level > 0) {
        // This is an inner node, insert the node into the next
        unsigned int idx;
        this->locate(key, idx);

        auto values = this->get_values<Address>();

        Node<K, V> subtree(fs, values[idx], this->length, this->container, this, idx);

        Status status = subtree.load();
        if (status < 0) return status;

        return subtree.insert(key, value, collide);
    }

    return this->insert_direct<V>(key, value);
}

template <typename K, typename V>
Status Node<K, V>::update(const K &key, const V *value) {
    if (this->hdr->num_values == 0) return Status::E_NOT_FOUND;
    if (this->hdr->num_values == 1) {
        assert(this->hdr->level == 0);

        auto keys = this->get_keys();
        if (key != keys[0]) return Status::E_NOT_FOUND;

        auto values = this->get_values<V>();
        if (!equiv_values(values, value)) return Status::E_NOT_FOUND;

        memcpy(values, value, this->get_record_size());

        return this->store();
    }

    if (this->hdr->level > 0) {
        auto values = this->get_values<Address>();

        unsigned int idx;
        this->locate(key, idx);

        Node<K, V> subtree(
            this->fs, values[idx], this->length, this->container, this, idx
        );

        Status status = subtree.load();
        if (status < Status::OK) return status;

        return subtree.update(key, value);
    }

    auto keys = this->get_keys();
    auto values = this->get_values<V>();

    unsigned int idx;
    Status status = this->locate_in_leaf(key, idx);
    if (status < Status::OK) return status;

    long lidx = idx;
    for (; lidx >= 0 && keys[lidx] == key && equiv_values(values + lidx, value); --lidx) {
        memcpy(this->get_value<V>(lidx), value, this->get_record_size());
    }

    status = this->store();
    if (lidx > 0 || this->prev() == 0 || status < Status::OK) return status;

    Node<K, V> sibling(
        this->fs, this->prev(), this->length, this->container, this, this->index_in_parent - 1
    );

    status = sibling.load();
    if (status < Status::OK) return status;

    status = sibling.update(key, value);
    if (status == Status::E_NOT_FOUND) return Status::OK;

    return status;
}

template <typename K, typename V>
template <typename R>
Status Node<K, V>::adopt(Node<K, V> *adoptee) {
    auto value_cap = this->get_cap<R>();
    auto num_left = adoptee->hdr->num_values;
    auto num_right = this->hdr->num_values;

    auto keys = this->get_keys();
    auto values = this->get_values<R>();

    if (this->hdr->num_values + adoptee->hdr->num_values > value_cap) return Status::E_CANT_ADOPT;

    memmove(keys + num_left, keys, num_right * sizeof(K));
    memmove(this->get_value<R>(num_left), values, num_right * this->get_record_size());

    memcpy(keys, adoptee->get_keys(), num_left * sizeof(K));
    memcpy(values, adoptee->get_values<R>(), num_left * this->get_record_size());

    this->hdr->num_values += num_left;
    this->prev() = adoptee->prev();

    // Save before altering the parent
    auto status = this->store();
    if (status < 0) return status;

    assert(this->parent);

    status = this->parent->remove_direct<Address>(adoptee->index_in_parent);
    if (status < 0) return status;

    return this->container->free({adoptee->addr, adoptee->length});
}

template <typename K, typename V>
template <typename R>
Status Node<K, V>::abduct_highest(Node<K, V> &node) {
    assert(node.index_in_parent < this->index_in_parent);

    --node.hdr->num_values;

    auto keys = this->get_keys();
    auto values = this->get_values<R>();

    // Make room for the new value and copy it over.
    memmove(keys + 1, keys, this->hdr->num_values * sizeof(K));
    memmove(this->get_value<R>(1), values, this->hdr->num_values * this->get_record_size());

    keys[0] = node.get_keys()[node.hdr->num_values];
    memcpy(values, node.get_value<R>(node.hdr->num_values), this->get_record_size());

    ++this->hdr->num_values;

    // Write the new nodes to disk
    Status status = this->store();
    if (status < 0) return status;

    status = node.store();
    if (status < 0) return status;

    // Update the key for the left node in the parent.
    auto parent_keys = this->parent->get_keys();
    auto node_keys = node.get_keys();
    parent_keys[node.index_in_parent] = node_keys[node.hdr->num_values - 1];
    return this->parent->store();
}

template <typename K, typename V>
template <typename R>
Status Node<K, V>::abduct_lowest(Node<K, V> &node) {
    assert(node.index_in_parent > this->index_in_parent);

    --node.hdr->num_values;

    auto keys = this->get_keys();
    auto victim_keys = node.get_keys();
    auto victim_values = node.get_values<R>();

    keys[this->hdr->num_values] = victim_keys[0];
    memcpy(this->get_value<R>(this->hdr->num_values), victim_values, this->get_record_size());

    ++this->hdr->num_values;

    memmove(victim_keys, victim_keys + 1, node.hdr->num_values * sizeof(K));
    memmove(victim_values, node.get_value<R>(1), node.hdr->num_values * this->get_record_size());

    Status status = this->store();
    if (status < 0) return status;

    status = node.store();
    if (status < 0) return status;

    auto parent_keys = this->parent->get_keys();
    parent_keys[this->index_in_parent] = keys[this->hdr->num_values - 1];
    return this->parent->store();
}

template <typename K, typename V>
template <typename R>
Status Node<K, V>::remove_direct(unsigned int idx) {
    assert(this->hdr->num_values > 0);
    assert(idx < this->hdr->num_values);

    Status status;

    // If there's a parent, then the index must be valid
    assert(this->parent == nullptr || this->index_in_parent < UINT_MAX);

    auto keys = this->get_keys();

    if (this->hdr->level > 0 && this->parent == nullptr && this->hdr->num_values == 2) {
        status = this->container->free({this->addr, this->length});

        auto values = this->get_values<Address>();

        const auto addr = values[1 - idx];
        Status also_status = this->container->update_root(addr);

        if (status >= Status::OK) return also_status;
        return status;
    }

    --this->hdr->num_values;
    memmove(keys + idx, keys + idx + 1, (this->hdr->num_values - idx) * sizeof(K));
    memmove(
        this->get_value<R>(idx),
        this->get_value<R>(idx + 1),
        (this->hdr->num_values - idx) * this->get_record_size()
    );

    auto value_cap = this->get_cap<R>();

    // If the node is full enough or is the only node in the tree, just save and return.
    if (this->hdr->num_values >= value_cap / 2 || this->parent == nullptr) {
        return this->store();
    }

    // Otherwise check if we can fondle the neighbors
    if (this->index_in_parent > 0) {
        Node<K, V> left_sibling(
            this->fs, this->parent->get_values<Address>()[this->index_in_parent - 1],
            this->length, this->container, this->parent, this->index_in_parent - 1
        );

        status = left_sibling.load();
        if (status < 0) return status;

        // Try full adoption first
        status = this->adopt<R>(&left_sibling);
        if (status != Status::E_CANT_ADOPT) return status;

        // Adoption is not possible, steal their highest value
        return this->abduct_highest<R>(left_sibling);
    }

    if (this->index_in_parent < this->parent->hdr->num_values - 1) {
        assert(this->parent->get_values<Address>()[this->index_in_parent] == this->addr);

        Node<K, V> right_sibling(
            this->fs, this->parent->get_values<Address>()[this->index_in_parent + 1],
            this->length, this->container, this->parent, this->index_in_parent + 1
        );

        status = right_sibling.load();
        if (status < 0) return status;

        // Try full adoption first
        status = right_sibling.adopt<R>(this);
        if (status != Status::E_CANT_ADOPT) return status;

        // Transfer this node's highest value
        return this->abduct_lowest<R>(right_sibling);
    }

    assert("unable to keep node size reasonable; tree is inconsistent" == nullptr);
    return Status::E_INTERNAL;
}

template<typename K, typename V>
Status Node<K, V>::remove(const K &key, V *value, bool strict) {
    if (this->hdr->num_values == 0) return Status::E_NOT_FOUND;
    if (this->hdr->num_values == 1) {
        assert(this->hdr->level == 0);
        auto keys = this->get_keys();
        if (!strict && key > keys[0]) return Status::E_NOT_FOUND;
        if (strict && key != keys[0]) return Status::E_NOT_FOUND;

        memcpy(value, this->get_value<V>(0), this->get_record_size());

        this->hdr->num_values = 0;

        return this->store();
    }

    if (this->hdr->level > 0) {
        // This is an inner node, look up the address of the next node
        auto values = this->get_values<Address>();

        unsigned int idx;
        this->locate(key, idx);

        Node<K, V> subtree(
            this->fs, values[idx], this->length, this->container, this, idx
        );

        Status status = subtree.load();
        if (status < 0) return status;

        return subtree.remove(key, value, strict);
    }

    unsigned int idx;
    Status stt = strict ? this->locate_in_leaf_strict(key, idx) : this->locate_in_leaf(key, idx);
    if (stt < 0) return stt;

    // This is a leaf, look up the value
    memcpy(value, this->get_value<V>(idx), this->get_record_size());

    // Now remove the entry.
    return this->remove_direct<V>(idx);
}

template<typename K, typename V>
Status Node<K, V>::get_last_leaf(Address &target) {
    Status status = this->load();
    if (status < 0) return status;

    if (this->hdr->level == 0) {
        target = this->addr;
        return Status::OK;
    }

    assert(this->hdr->num_values > 0);

    auto values = this->get_values<Address>();
    unsigned int idx = this->hdr->num_values - 1;
    auto addr = values[idx];

    if (this->hdr->level == 1) {
        target = addr;
        return Status::OK;
    }

    Node<K, V> subtree(this->fs, addr, this->length, this->container, this, idx);
    return subtree.get_last_leaf(target);
}

template <typename K, typename V>
template <typename P>
Status Node<K, V>::destroy(EntryConsumer<V, P> destroyer, P &pl) {
    auto status = this->fs->free_blocks({this->addr, this->length});
    if (status < Status::OK) return status;

    if (this->hdr->level > 0) {
        auto values = this->get_values<Address>();

        for (long i = 0; i < this->hdr->num_values; ++i) {
            Node<K, V> subtree(this->fs, values[i], this->length, this->container, this, i);

            status = subtree.load();
            if (status < Status::OK) return status;

            status = subtree.destroy(destroyer, pl);
            if (status < Status::OK) return status;
        }

        auto prev = this->prev();
        if (prev == 0) return Status::OK;

        Node<K, V> sibling(
            this->fs, prev, this->length, this->container, this, this->index_in_parent - 1
        );

        status = sibling.load();
        if (status < Status::OK) return status;

        return sibling.destroy(destroyer, pl);
    }

    for (long i = 0; i < this->hdr->num_values; ++i) {
        do status = destroyer(*this->get_value<V>(i), pl);
        while (status == Status::RETRY);

        if (status == Status::STOP) return Status::E_STOPPED;
    }

    return Status::OK;
}

template <typename K, typename V>
Status Node<K, V>::count_used_space(Size &size) {
    size += this->length;

    if (this->hdr->level == 1) {
        size += this->hdr->num_values * this->length;
    } else if (this->hdr->level > 1) {
        const auto values = this->get_values<Address>();

        for (long i = 0; i < this->hdr->num_values; ++i) {
            Node<K, V> subtree(this->fs, values[i], this->length, this->container, this, i);

            auto status = subtree.load();
            if (status < Status::OK) return status;

            status = subtree.count_used_space(size);
            if (status < Status::OK) return status;
        }
    }

    return Status::OK;
}

template<typename K, typename V>
int Node<K, V>::pretty_print(UNUSED char *buf, UNUSED Size len, UNUSED bool reload) {
    return 0;
}

}
