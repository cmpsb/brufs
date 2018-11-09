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

#include <cassert>
#include <cstring>
#include <cstdlib>
#include <new>

#include "../Status.hpp"
#include "../Extent.hpp"

namespace Brufs {

class Brufs;

namespace BmTree {

/**
 * Allocates one or more blocks to store tree data in.
 *
 * @param fs the filesystem to allocate the blocks from
 * @param length the number of bytes the allocation should contain
 * @param target where to store allocation information
 *
 * @return a status code whether allocation succeeded or failed
 */
using Allocator = Status (*)(Brufs &fs, Size length, Extent &target);

/**
 * Allocates blocks using the usual free blocks tree.
 */
static inline Status ALLOC_NORMAL(Brufs &, Size, Extent &);

using Deallocator = Status (*)(Brufs &fs, const Extent &ext);

static inline Status DEALLOC_NORMAL(Brufs &, const Extent &);

template <typename V, typename P>
using EntryConsumer = Status (*)(V &item, P pl);

template <typename V>
using ContextlessEntryConsumer = Status (*)(V &item);

template <typename V>
bool equiv_values(const V *current, const V *replacement) {
    (void) current;
    (void) replacement;

    return true;
}

/**
 * The header of a Bm+tree node.
 */
struct Header {
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

    Header(uint8_t level = 0, uint32_t num_values = 0) :
        magic {'B', '+'},
        level(level),
        size(sizeof(Header)),
        num_values(num_values)
    {}
};
static_assert(std::is_standard_layout<Header>::value);
static_assert(sizeof(Header) % 8 == 0);

template <typename K, typename V>
class Node;

/**
 * A B+tree with flexible index prediction.
 *
 * @tparam K the key type
 * @tparam V the value type in the leaves
 * @tparam A the Allocator function to use
 * @tparam C the comparator function to use
 */
template <typename K, typename V>
class BmTree {
private:
    /**
     * The filesystem the Bm+tree resides on.
     */
    Brufs *fs;

    /**
     * The size of a node in bytes.
     */
    Size length;

    /**
     * The maximum number of attempts the tree may make to insert new values in a nearly full tree.
     * This will also limit the number of levels in the tree.
     */
    unsigned int max_level;

    unsigned int value_size;

    /**
     * The Allocator to use.
     */
    Allocator alloctr;

    Deallocator dealloctr;

    /**
     * The root of the tree.
     */
    Node<K, V> root;
    friend Node<K, V>;

    /**
     * Allocates a block for the tree.
     *
     * @param length the length of the block in bytes
     * @param target where to write the block extent
     *
     * @return a status return code
     */
    Status alloc(Size length, Extent &target);

    Status free(const Extent &ext);

public:
    /**
     * Creates a Bm+tree by loading an existing tree from disk.
     *
     * @param fs the filesystem the tree resides on
     * @param addr the address of the root node of the tree
     * @param length the size of each node in the tree
     * @param max_level the maximum level of nodes in the tree
     */
    BmTree(Brufs *fs, Address addr, Size length,
           Allocator alloc = ALLOC_NORMAL, Deallocator dealloc = DEALLOC_NORMAL,
           unsigned int max_level = 5);

    BmTree(Brufs *fs, Size length,
           Allocator alloc = ALLOC_NORMAL, Deallocator dealloc = DEALLOC_NORMAL,
           unsigned int max_level = 5);

    BmTree(const BmTree<K, V> &other);

    BmTree<K, V> &operator=(const BmTree<K, V> &other);

    Status init(Size new_length = 0);

    Status search(const K key, V *value, bool exact = false);
    Status search(const K key, V &value, bool exact = false) {
        return this->search(key, &value, exact);
    }

    template <typename P>
    int search(const K key, P *value, int max, bool exact = false);

    Status get_first(V *value);
    Status get_first(V &value) { return this->get_first(&value); }

    Status get_last(V *value);
    Status get_last(V &value) { return this->get_last(&value); }

    Status insert(const K key, const V *value, bool collide = false);
    Status insert(const K key, const V &value, bool collide = false) {
        return this->insert(key, &value, collide);
    }

    Status update(const K key, const V *value);
    Status update(const K key, const V &value) { return this->update(key, &value); }

    Status remove(const K key, V *value, bool exact = false);
    Status remove(const K key, V &value, bool exact = false) {
        return this->remove(key, &value, exact);
    }

    Status count_values(Size &count);
    Status count_used_space(Size &size);

    template <typename P>
    Status destroy(EntryConsumer<V, P> destroyer, P pl);
    Status destroy(ContextlessEntryConsumer<V> destroyer);
    Status destroy();

    template <typename P>
    Status walk(EntryConsumer<V, P> consumer, P pl);

    Status walk(ContextlessEntryConsumer<V> consumer);

    int pretty_print_root(char *buf, Size len);

    /**
     * @brief [brief description]
     * @details [long description]
     *
     * @param new_addr [description]
     */
    Status update_root(Address new_addr, Size length = 0);

    virtual Status on_root_change(Address new_root) {
        (void) new_root;
        return Status::OK;
    }

    void set_value_size(unsigned int value_size) {
        this->value_size = value_size;
    }

    auto get_value_size() const {
        return this->value_size;
    }
};

/**
 * A node in the Bm+tree.
 *
 * If this is not a leaf node, then the values will have the Brufs::address type.
 * The V template parameter should still be the leaf type, however.
 *
 * @tparam K the key type
 * @tparam V the value type in the leaves
 * @tparam A the allocator function to use
 * @tparam C the comparator function to use
 */
template <typename K, typename V>
struct Node {
    /**
     * The filesystem this node resides on.
     */
    Brufs *fs;

    /**
     * The address this node starts at.
     */
    Address addr;

    /**
     * The size of this node in bytes.
     */
    Size length;

    /**
     * The containing BmTree keeping track of the root of the tree.
     */
    BmTree<K, V> *container;

    /**
     * The memory buffer the actual node data resides in.
     */
    char *buf;

    /**
     * The portion of the buffer representing this node's on-disk header.
     */
    Header *hdr;

    /**
     * The parent of this node.
     */
    class Node<K, V> *parent;

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
    Node(Brufs *fs, Address addr, Size length, BmTree<K, V> *container,
         Node<K, V> *parent = nullptr, unsigned int index_in_parent = UINT_MAX);

    /**
     * Creates a new node not backed by any on-disk structure.
     *
     * @param fs the filesystem the tree belongs to
     * @param container the managing container representing the entire tree
     */
    Node(Brufs *fs, BmTree<K, V> *container);

    /**
     * Copies another node.
     * This creates another view into the same on-disk structure.
     *
     * @param other the node to copy
     */
    Node(const Node<K, V> &other);

    /**
     * Destructs the node, while retaining the data on-disk.
     */
    ~Node();

    /**
     * Copies another node into this instance.
     * This copies the other node's view into this node, it does not copy the on-disk structure.
     *
     * @param other the node to copy
     */
    Node<K, V> &operator=(const Node<K, V> &other);

    /**
     * Initializes the node.
     *
     * @return a status code
     */
    Status init();

    /**
     * Returns the size of the value type in bytes.
     *
     * @return the size of the value type
     */
    auto get_record_size();

    /**
     * Returns the maximum possible amount of values (and thus also keys) this node can contain.
     *
     * @tparam R the type of value this node stores
     */
    auto get_cap();

    /**
     * Returns a pointer into the in-memory buffer to the first value in the node.
     *
     * @tparam R the type of value this node stores
     * @return a pointer to the first value
     */
    template <typename R>
    R *get_values();

    template <typename R>
    R *get_value(unsigned int idx);

    /**
     * Returns a pointer into the in-memory buffer to the first key in the node.
     *
     * @return a pointer to the first key
     */
    K *get_keys();

    /**
     * Returns a pointer to the link to the previous node.
     */
    Address *get_link();

    /**
     * Returns a reference to the address of the previous node.
     */
    Address &prev();

    /**
     * Loads the node from disk.
     *
     * @return a status code
     */
    Status load();

    /**
     * Writes the node to disk.
     *
     * @return a status code
     */
    Status store();

    /**
     * Finds the index of the best match for the search key.
     *
     * @param key the key to search for
     * @param result where to store the index
     *
     * @return E_NOT_FOUND if no match could be found, OK otherwise
     */
    void locate(const K &key, unsigned int &result);

    /**
     * Finds the index of the best match for the search key.
     *
     * @param key the key to search for
     * @param result where to store the index
     *
     * @return E_NOT_FOUND if no match could be found, OK otherwise
     */
    Status locate_in_leaf(const K &key, unsigned int &result);

    /**
     * Finds the index of the exact match for the search key.
     *
     * @param key the key to search for
     * @param result where to store the index
     *
     * @return E_NOT_FOUND if no match could be found, OK otherwise
     */
    Status locate_in_leaf_strict(const K &key, unsigned int &result);

    /**
     * Looks up the value associated with the key.
     *
     * @param key the key to search for
     * @param value where to store the value
     * @param strict whether to only return success if the key matches exactly
     *
     * @return a status code; OK if the key was found, E_NOT_FOUND if it wasn't
     */
    Status search(const K &key, V *value, bool exact);

    int search_all(const K &key, uint8_t *value, int max, bool exact);
    int copy_while(const K &key, uint8_t *value, unsigned int start, int max, bool exact);

    Status get_first(V *value);
    Status get_last(V *value);

    Status insert_initial(const K &key, Address left, Address right);

    template <typename R>
    Status split(const K &key, const R *value, unsigned int idx);

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
    Status insert_direct(const K &key, const R *value, bool collide = false);

    template <typename R>
    Status insert_direct_at(const K &key, const R *value, unsigned int idx, bool collide = false);

    /**
     * Inserts a value in this part of the (sub-)tree.
     *
     * @param key the key to insert the value under
     * @param value the value to insert
     *
     * @return a status code
     */
    Status insert(const K &key, const V *value, bool collide);

    Status update(const K &key, const V *value);

    Status remove(const K &key, V *value, bool exact);

    template <typename R>
    Status adopt(Node *adoptee);

    template <typename R>
    Status abduct_highest(Node &node);

    template <typename R>
    Status abduct_lowest(Node &node);

    template <typename R>
    Status remove_direct(unsigned int idx);

    Status get_last_leaf(Address &target);

    template <typename P>
    Status destroy(EntryConsumer<V, P> destroyer, P &pl);

    Status count_used_space(Size &size);

    int pretty_print(char *buf, Size len, bool reload = true);
    //     (void) buf;
    //     (void) len;
    //     (void) reload;
    //     return 0;
    // }
};

}}
