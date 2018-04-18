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

#include <cstdio>

#include "brufs.hpp"
#include "rtstructures.hpp"

namespace brufs { namespace bmtree {

template<typename K, typename V, allocator A>
node<K, V, A>::node(brufs &fs, address addr, size length, bmtree<K, V, A> *container, 
                    node<K, V, A> *parent, unsigned int index_in_parent) :
    fs(fs), addr(addr), length(length), container(container), parent(parent), 
    index_in_parent(index_in_parent)
{
    this->buf = static_cast<char *>(malloc(length));
    this->hdr = reinterpret_cast<header *>(this->buf);
}

template<typename K, typename V, allocator A>
node<K, V, A>::node(brufs &fs, bmtree<K, V, A> *container) :
    fs(fs), addr(0), length(0), container(container), parent(nullptr), index_in_parent(UINT_MAX)
{
    this->buf = NULL;
    this->hdr = NULL;
}

template<typename K, typename V, allocator A>
node<K, V, A>::node(const node<K, V, A> &other) :
    node(other.fs, other.addr, other.length, other.container, other.parent)
{}

template<typename K, typename V, allocator A>
node<K, V, A> &node<K, V, A>::operator=(const node<K, V, A> &other) {
    if (this->length != other.length) {
        this->~node();
        this->buf = static_cast<char *>(malloc(other.length));
        this->hdr = reinterpret_cast<header *>(this->buf);
    }

    this->fs = other.fs;
    this->addr = other.addr;
    this->length = other.length;
    this->container = other.container;
    this->parent = other.parent;

    return *this;
}

template<typename K, typename V, allocator A>
template<typename R>
auto node<K, V, A>::get_cap() {
    // return (this->length - sizeof(header) - sizeof(address)) / (sizeof(K) + sizeof(R));
    return 4u;
}

template<typename K, typename V, allocator A>
template<typename R>
R *node<K, V, A>::get_values() {
    return reinterpret_cast<R *>(this->buf + sizeof(header) + this->get_cap<R>() * sizeof(K));
}

template<typename K, typename V, allocator A>
K *node<K, V, A>::get_keys() {
    return reinterpret_cast<K *>(this->buf + sizeof(header));
}

template<typename K, typename V, allocator A>
address *node<K, V, A>::get_link() {
    return reinterpret_cast<address *>(this->buf + this->length - sizeof(address));
}

template <typename K, typename V, allocator A>
address &node<K, V, A>::prev() {
    return *this->get_link();
}

template<typename K, typename V, allocator A>
status node<K, V, A>::load() {
    assert(this->buf);

    ssize status = dread(this->fs.get_disk(), this->buf, this->length, this->addr);
    if (status < 0) return static_cast<::brufs::status>(status);

    return status::OK;
}

template<typename K, typename V, allocator A>
status node<K, V, A>::store() {
    ssize status = dwrite(this->fs.get_disk(), this->buf, this->length, this->addr);
    if (status < 0) return static_cast<::brufs::status>(status);

    return status::OK;
}

template<typename K, typename V, allocator A>
status node<K, V, A>::locate(const K &key, unsigned int &result) {
    assert(this->hdr->num_values > 0);

    auto keys = this->get_keys();

    unsigned int i = 0;
    for (; i < (this->hdr->num_values - 1) && key > keys[i]; ++i);
        
    if (i >= (this->hdr->num_values - 1) && key > keys[i]) ++i;

    result = i;

    return status::OK;
}

template<typename K, typename V, allocator A>
status node<K, V, A>::locate_in_leaf(const K &key, unsigned int &result) {
    assert(this->hdr->level == 0);
    assert(this->hdr->num_values > 0);

    auto keys = this->get_keys();

    unsigned int i = 0;
    for (; i < this->hdr->num_values && key > keys[i]; ++i);

    result = i;
    if (i >= this->hdr->num_values) return status::E_NOT_FOUND;

    return status::OK;
}

template<typename K, typename V, allocator A>
status node<K, V, A>::search(const K &key, V &value) {
    status stt = this->load();
    if (stt < 0) return stt;

    if (this->hdr->num_values == 0) return status::E_NOT_FOUND;
    if (this->hdr->num_values == 1) {
        assert(this->hdr->level == 0);
        auto keys = this->get_keys();
        if (key > keys[0]) return status::E_NOT_FOUND;

        auto values = this->get_values<V>();
        value = values[0];

        return status::OK;
    }

    if (this->hdr->level > 0) {
        // This is an inner node, look up the address of the next node
        auto values = this->get_values<address>();

        unsigned int idx;
        this->locate(key, idx);

        node<K, V, A> subtree(
            this->fs, values[idx], this->length, this->container, this, idx
        );

        return subtree.search(key, value);
    }

    unsigned int idx;
    stt = this->locate_in_leaf(key, idx);
    if (stt < 0) return stt;

    // This is a leaf, look up the value
    auto values = this->get_values<V>();

    value = values[idx];
    return status::OK;
}

template<typename K, typename V, allocator A>
status node<K, V, A>::insert_initial(const K &key, address left, address right) {
    this->hdr->num_values = 2;

    auto keys = this->get_keys();
    auto values = this->get_values<address>();

    keys[0] = key;
    values[0] = left;
    values[1] = right;

    return this->store();
}

template<typename K, typename V, allocator A>
template<typename R>
status node<K, V, A>::split() {
    status status;

    auto keys = this->get_keys();
    auto values = this->get_values<R>();

    auto num_left = this->hdr->num_values / 2;
    auto num_right = this->hdr->num_values - num_left;

    this->hdr->num_values = num_right;

    extent sibling_extent;
    status = A(this->fs, this->length, sibling_extent);
    if (status < 0) return status;

    node<K, V, A> sibling(
        this->fs, sibling_extent.offset, this->length, this->container, this
    );

    memset(sibling.hdr, 0, sizeof(header));
    sibling.hdr->level = this->hdr->level;
    sibling.hdr->num_values = num_left;

    auto sibling_keys = sibling.get_keys();
    auto sibling_values = sibling.get_values<R>();

    memcpy(sibling_keys, keys, num_left * sizeof(K));
    memcpy(sibling_values, values, num_left * sizeof(R));

    sibling.prev() = this->prev();
    this->prev() = sibling_extent.offset;

    status = sibling.store();
    if (status < 0) {
        this->fs.free_blocks(sibling_extent);
        return status;
    }

    memmove(keys, keys + num_left, num_right * sizeof(K));
    memmove(values, values + num_left, num_right * sizeof(R));

    status = this->store();
    if (status < 0) {
        this->fs.free_blocks(sibling_extent);
        return status;
    }

    if (this->parent != nullptr) {
        do {
            status = this->parent->insert_direct<address>(
                sibling_keys[num_left - 1], sibling_extent.offset
            );
            if (status < 0) {
                this->fs.free_blocks(sibling_extent);
                return status;
            }
        } while (status == status::RETRY);

        return status::RETRY;
    } else {
        extent parent_extent;
        status = A(this->fs, this->length, parent_extent);
        if (status < 0) {
            this->fs.free_blocks(sibling_extent);
            return status;
        }

        // Create a new parent
        node<K, V, A> pstor(
            this->fs, parent_extent.offset, this->length, this->container
        );
        this->parent = &pstor;

        memset(this->parent->hdr, 0, sizeof(header));
        this->parent->hdr->level = this->hdr->level + 1;

        status = this->parent->insert_initial(
            sibling_keys[num_left - 1], sibling_extent.offset, this->addr
        );
        if (status < 0) {
            this->fs.free_blocks(sibling_extent);
            this->fs.free_blocks(parent_extent);
            return status;
        }

        this->container->update_root(parent_extent.offset);
        return status::RETRY;
    }
}

template<typename K, typename V, allocator A>
template <typename R>
status node<K, V, A>::insert_direct(const K &key, const R &value, bool reload) {
    status status;

    if (reload) {
        status = this->load();
        if (status < 0) return status;
    }

    auto key_cap = this->get_cap<R>();

    if (this->hdr->num_values >= key_cap) return this->split<R>();

    unsigned int idx;
    status = this->locate(key, idx);

    auto keys = this->get_keys();
    auto values = this->get_values<R>();

    memmove(keys + idx + 1, keys + idx, (this->hdr->num_values - idx) * sizeof(K));
    keys[idx] = key;

    memmove(values + idx + 1, values + idx, (this->hdr->num_values - idx) * sizeof(R));
    values[idx] = value;

    ++this->hdr->num_values;

    return this->store();
}

template<typename K, typename V, allocator A>
status node<K, V, A>::insert(const K &key, const V &value) {
    status status = this->load();
    if (status < 0) return status;

    if (this->hdr->num_values == 0) {
        assert(this->hdr->level == 0);

        auto keys = this->get_keys();
        auto values = this->get_values<V>();

        keys[0] = key;
        values[0] = value;

        this->hdr->num_values = 1;
        return this->store();
    }
        
    if (this->hdr->level > 0) {
        // This is an inner node, insert the node into the next
        unsigned int idx;
        this->locate(key, idx);

        auto values = this->get_values<address>();

        node<K, V, A> subtree(fs, values[idx], this->length, this->container, this, idx);

        return subtree.insert(key, value);
    }

    return this->insert_direct<V>(key, value, false);
}

template <typename K, typename V, allocator A>
template <typename R>
status node<K, V, A>::adopt(node<K, V, A> *adoptee) {
    status status;

    auto value_cap = this->get_cap<R>();
    auto num_left = adoptee->hdr->num_values;
    auto num_right = this->hdr->num_values;

    auto keys = this->get_keys();
    auto values = this->get_values<R>();

    if (this->hdr->num_values + adoptee->hdr->num_values > value_cap) {
        memmove(keys + num_left, keys, num_right * sizeof(K));
        memmove(values + num_left, values, num_right * sizeof(R));

        memcpy(keys, adoptee->get_keys(), num_left * sizeof(K));
        memcpy(values, adoptee->get_values<R>(), num_left * sizeof(R));

        this->prev() = adoptee->prev();

        // Save before altering the parent
        status = this->store();
        if (status < 0) return status;

        assert(this->parent);

        status = this->parent->remove_direct<address>(adoptee->index_in_parent);
        if (status < 0) return status;

        this->fs.free_blocks(extent { adoptee->addr, adoptee->length });

        return status::OK;
    }

    return status::E_CANT_ADOPT;
}

template <typename K, typename V, allocator A>
template <typename R>
status node<K, V, A>::abduct_highest(node<K, V, A> *node) {
    assert(node != nullptr);

    --node->hdr->num_values;

    auto keys = this->get_keys();
    auto values = this->get_values<R>();

    // Make room for the new value and copy it over.
    memmove(keys + 1, keys, this->hdr->num_values * sizeof(K));
    memmove(values + 1, values, this->hdr->num_values * sizeof(R));

    keys[0] = node->get_keys()[node->hdr->num_values];
    values[0] = node->get_values<R>()[node->hdr->num_values];

    ++this->hdr->num_values;

    // Write the new nodes to disk
    status status = this->store();
    if (status < 0) return status;

    status = node->store();
    if (status < 0) return status;

    // Update the key for the left node in the parent.
    this->parent->get_keys()[node->index_in_parent] = 
            node->get_values<R>()[node->hdr->num_values - 1];
    return this->parent->store();
}

template <typename K, typename V, allocator A>
template <typename R>
status node<K, V, A>::remove_direct(unsigned int idx) {
    status status;

    // If there's a parent, then the index must be valid
    assert(this->parent == nullptr || this->index_in_parent < UINT_MAX);

    auto keys = this->get_keys();
    auto values = this->get_values<R>();

    --this->hdr->num_values;
    memmove(keys + idx, keys + idx + 1, (this->hdr->num_values - idx) * sizeof(K));
    memmove(values + idx, values + idx + 1, (this->hdr->num_values - idx) * sizeof(R));

    auto value_cap = this->get_cap<R>();

    // This root is nearly empty, promote the only child as the new root.
    if (this->hdr->level > 0 && this->parent == nullptr && this->hdr->num_values == 1) {
        this->container->update_root(values[0]);
        this->fs.free_blocks(extent { this->addr, this->length });
        return status::OK;
    }

    // If the node is full enough or is the only node in the tree, just save and return.
    if (this->hdr->num_values >= value_cap / 2 || this->parent == nullptr) {
        printf("node is full enough; not modifying: %u > %u or no parent %p\n",
            this->hdr->num_values,
            value_cap / 2,
            this->parent
        );
        return this->store();
    }

    // Otherwise check if we can fondle the neighbors
    if (this->index_in_parent > 0) {
        node<K, V, A> left_sibling(
            this->fs, this->parent->get_values<address>()[this->index_in_parent - 1],
            this->length, this->container, this->parent, this->index_in_parent - 1
        );

        status = left_sibling.load();
        if (status < 0) return status;

        printf("try adopt left\n");

        // Try full adoption first
        status = this->adopt<R>(&left_sibling);
        if (status != status::E_CANT_ADOPT) return status;

        printf("can't adopt left\n");

        // Adoption is not possible, steal their highest value
        return this->abduct_highest<R>(&left_sibling);
    }

    if (this->index_in_parent < this->parent->hdr->num_values - 1) {
        node<K, V, A> right_sibling(
            this->fs, this->parent->get_values<address>()[this->index_in_parent + 1],
            this->length, this->container, this->parent, this->index_in_parent + 1
        );

        status = right_sibling.load();
        if (status < 0) return status;

        printf("try adopt right\n");

        // Try full adoption first
        status = right_sibling.adopt<R>(this);
        if (status != status::E_CANT_ADOPT) return status;

        printf("right can't adopt\n");

        // Transfer this node's highest value
        return right_sibling.abduct_highest<R>(this);
    }

    assert("unable to keep node size reasonable; tree is inconsistent" == nullptr);
    return status::E_INTERNAL;
}

template<typename K, typename V, allocator A>
status node<K, V, A>::remove(const K &key, V &value) {
    status status = this->load();
    if (status < 0) return status;

    if (this->hdr->num_values == 0) return status::E_NOT_FOUND;
    if (this->hdr->num_values == 1) {
        assert(this->hdr->level == 0);
        auto keys = this->get_keys();
        if (key > keys[0]) return status::E_NOT_FOUND;

        auto values = this->get_values<V>();
        value = values[0];

        this->hdr->num_values = 0;

        return this->store();
    }

    if (this->hdr->level > 0) {
        // This is an inner node, look up the address of the next node
        auto values = this->get_values<address>();

        unsigned int idx;
        this->locate(key, idx);

        node<K, V, A> subtree(
            this->fs, values[idx], this->length, this->container, this, idx
        );

        return subtree.remove(key, value);
    }

    unsigned int idx;
    status = this->locate_in_leaf(key, idx);
    if (status < 0) return status;

    // This is a leaf, look up the value
    auto values = this->get_values<V>();

    value = values[idx];

    // Now remove the entry.
    return this->remove_direct<V>(idx);
}

template<typename K, typename V, allocator A>
status node<K, V, A>::get_last_leaf(address &target) {
    status status = this->load();
    if (status < 0) return status;

    if (this->hdr->level == 0) {
        target = this->addr;
        return status::OK;
    }

    assert(this->hdr->num_values > 0);

    auto values = this->get_values<address>();
    unsigned int idx = this->hdr->num_values - 1;
    auto addr = values[idx];

    if (this->hdr->level == 1) {
        target = addr;
        return status::OK;
    }

    node<K, V, A> subtree(this->fs, addr, this->length, this->container, this, idx);
    return subtree.get_last_leaf(target);
}

template<typename K, typename V, allocator A>
int node<K, V, A>::pretty_print(char *buf, size len, bool reload) {
    if (reload) {
        this->load();
    }

    int total = snprintf(buf, len, "class n%lX << %s >>\n",
        this->addr, this->hdr->level == 0 ? "(L,#228B22)" : "(I,#6B4423)"
    );

    if (this->hdr->level == 0) {
        auto keys = this->get_keys();
        auto values = this->get_values<V>();

        for (auto i = 0; i < this->hdr->num_values; ++i) {
            total += snprintf(buf + total, len - total, "n%lX --> %lu : > %lu\n", this->addr, values[i], keys[i]);
        }
    } else {
        auto keys = this->get_keys();
        auto values = this->get_values<address>();

        for (auto i = 0; i < this->hdr->num_values; ++i) {
            total += snprintf(buf + total, len - total, "n%lX --> n%lX : > %lu\n", this->addr, values[i], keys[i]);
        }

        for (auto i = 0; i < this->hdr->num_values; ++i) {
            node child(this->fs, values[i], this->length, this->container, this, i);
            total += child.pretty_print(buf + total, len - total);
        }
    }

    return total;
}

}}
