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

namespace brufs { namespace bmtree {
/**
 * Allocates one or more blocks to store tree data in.
 * 
 * @param fs the filesystem to allocate the blocks from
 * @param length the number of bytes the allocation should contain
 * @param target where to store allocation information
 * 
 * @return a status code whether allocation succeeded or failed
 */
using allocator = status (*)(brufs &fs, size length, extent &target);

/**
 * Allocates blocks using the usual free blocks tree.
 */
static status ALLOC_NORMAL(brufs &fs, size length, extent &target) {
    return fs.allocate_blocks(length, target);
}

/**
 * The header of a Bm+tree node.
 */
struct header {
    /**
     * The magic header "B+" marking this as a Bm+tree.
     */
    uint8_t magic[2];

    /**
     * The current level, 0 if this is a node of leaves.
     */
    uint8_t level;

    /**
     * Size of the header in bytes. Must be a multiple of 8.
     */
    uint8_t size;

    /**
     * The number of keys in the current node.
     */
    uint32_t num_values;

    header(uint8_t level = 0, uint32_t num_values = 0) : 
        magic {'B', '+'}, level(level), size(sizeof(header)), num_values(num_values)
    {}
};
static_assert(std::is_standard_layout<header>::value);
static_assert(sizeof(header) % 8 == 0);

template <typename K, typename V, allocator A>
class node;

/**
 * A B+tree with flexible index prediction.
 * 
 * @tparam K the key type
 * @tparam V the value type in the leaves
 * @tparam A the allocator function to use
 * @tparam C the comparator function to use
 */
template <typename K, typename V, allocator A = ALLOC_NORMAL>
class bmtree {
private:
    /**
     * The filesystem the Bm+tree resides on.
     */
    brufs &fs;

    /**
     * The size of a node in bytes.
     */
    size length;

    /**
     * The maximum number of attempts the tree may make to insert new values in a nearly full tree.
     * This will also limit the number of levels in the tree.
     */
    unsigned int max_level;

    /**
     * The root of the tree.
     */
    node<K, V, A> root;
    friend node<K, V, A>;

    /**
     * @brief [brief description]
     * @details [long description]
     * 
     * @param new_addr [description]
     */
    void update_root(address new_addr);

public:
    /**
     * Creates a Bm+tree by loading an existing tree from disk.
     * 
     * @param fs the filesystem the tree resides on
     * @param addr the address of the root node of the tree
     * @param length the size of each node in the tree
     * @param max_level the maximum level of nodes in the tree
     */
    bmtree(brufs &fs, address addr, size length, unsigned int max_level = 5);

    bmtree(brufs &fs, size length, unsigned int max_level = 5);

    bmtree(const bmtree<K, V, A> &other);

    bmtree<K, V, A> &operator=(const bmtree<K, V, A> &other);

    status init();

    status search(const K key, V &value, bool strict = false);

    status insert(const K key, const V value);

    status remove(const K key, V &value, bool strict = false);

    status count_values(size &count);

    int pretty_print_root(char *buf, size len);

    virtual void on_root_change(address new_root) {
        (void) new_root;
    }
};

/**
 * A node in the Bm+tree.
 * 
 * If this is not a leaf node, then the values will have the brufs::address type.
 * The V template parameter should still be the leaf type, however.
 * 
 * @tparam K the key type
 * @tparam V the value type in the leaves
 * @tparam A the allocator function to use
 * @tparam C the comparator function to use
 */
template <typename K, typename V, allocator A>
struct node {
    /**
     * The filesystem this node resides on.
     */
    brufs &fs;

    /**
     * The address this node starts at.
     */
    address addr;

    /**
     * The size of this node in bytes.
     */
    size length;

    /**
     * The containing bmtree keeping track of the root of the tree.
     */
    bmtree<K, V, A> *container;

    /**
     * The memory buffer the actual node data resides in.
     */
    char *buf;

    /**
     * The portion of the buffer representing this node's on-disk header.
     */
    header *hdr;

    /**
     * The parent of this node.
     */
    class node<K, V, A> *parent;

    /**
     * The index of the address in the parent pointing to this node.
     */
    unsigned int index_in_parent;

    /**
     * Creates a new node.
     * 
     * @param fs the filesystem the node belongs to
     * @param addr the address of the node's on-disk data
     * @param length the size, in bytes, of the node data
     * @param container the managing container representing the entire tree
     * @param parent the parent of this node (if any; NULL if this is the root)
     */
    node(brufs &fs, address addr, size length, bmtree<K, V, A> *container, 
         node<K, V, A> *parent = nullptr, unsigned int index_in_parent = UINT_MAX);

    /**
     * Creates a new node not backed by any on-disk structure.
     * 
     * @param fs the filesystem the tree belongs to
     * @param container the managing container representing the entire tree
     */
    node(brufs &fs, bmtree<K, V, A> *container);

    /**
     * Copies another node.
     * This creates another view into the same on-disk structure.
     * 
     * @param other the node to copy
     */
    node(const node<K, V, A> &other);

    /**
     * Destructs the node, while retaining the data on-disk.
     */
    ~node() {
        free(this->buf);
    }

    /**
     * Copies another node into this instance.
     * This copies the other node's view into this node, it does not copy the on-disk structure.
     * 
     * @param other the node to copy
     */
    node<K, V, A> &operator=(const node<K, V, A> &other);

    /**
     * Returns the maximum possible amount of values (and thus also keys) this node can contain.
     * 
     * @tparam R the type of value this node stores
     */
    template <typename R>
    auto get_cap();

    /**
     * Returns a pointer into the in-memory buffer to the first value in the node.
     * 
     * @tparam R the type of value this node stores
     * @return a pointer to the first value
     */
    template <typename R>
    R *get_values();

    /**
     * Returns a pointer into the in-memory buffer to the first key in the node.
     * 
     * @return a pointer to the first key
     */
    K *get_keys();

    /**
     * Returns a pointer to the link to the previous node.
     */
    address *get_link();

    /**
     * Returns a reference to the address of the previous node.
     */
    address &prev();
    
    /**
     * Loads the node from disk.
     *
     * @return a status code
     */
    status load();

    /**
     * Writes the node to disk.
     * 
     * @return a status code
     */
    status store();

    /**
     * Finds the index of the best match for the search key.
     * 
     * @param key the key to search for
     * @param result where to store the index
     * 
     * @return E_NOT_FOUND if no match could be found, OK otherwise
     */
    status locate(const K &key, unsigned int &result);

    /**
     * Finds the index of the best match for the search key.
     * 
     * @param key the key to search for
     * @param result where to store the index
     * 
     * @return E_NOT_FOUND if no match could be found, OK otherwise
     */
    status locate_in_leaf(const K &key, unsigned int &result);

    /**
     * Finds the index of the exact match for the search key.
     * 
     * @param key the key to search for
     * @param result where to store the index
     * 
     * @return E_NOT_FOUND if no match could be found, OK otherwise
     */
    status locate_in_leaf_strict(const K &key, unsigned int &result);

    /**
     * Looks up the value associated with the key.
     * 
     * @param key the key to search for
     * @param value where to store the value
     * @param strict whether to only return success if the key matches exactly
     * 
     * @return a status code; OK if the key was found, E_NOT_FOUND if it wasn't
     */
    status search(const K &key, V &value, bool strict);

    status insert_initial(const K &key, address left, address right);

    template <typename R>
    status split(const K &key, const R &value);

    /**
     * Inserts a value in this node, without walking the rest of the tree.
     * 
     * @param key the key to insert the value under
     * @param value the value to insert
     * @tparam R the type of value to insert; addresses for inner nodes and Vs for leaves
     * 
     * @return a status code
     */
    template <typename R>
    status insert_direct(const K &key, const R &value);

    /**
     * Inserts a value in this part of the (sub-)tree.
     * 
     * @param key the key to insert the value under
     * @param value the value to insert
     * 
     * @return a status code
     */
    status insert(const K &key, const V &value);

    status remove(const K &key, V &value, bool strict);

    template <typename R>
    status adopt(node *adoptee);

    template <typename R>
    status abduct_highest(node &node);

    template <typename R>
    status abduct_lowest(node &node);

    template <typename R>
    status remove_direct(unsigned int idx);

    status get_last_leaf(address &target);

    int pretty_print(char *buf, size len, bool reload = true);
};

}}
