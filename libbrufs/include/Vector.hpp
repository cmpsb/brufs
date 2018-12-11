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
#include <cstdlib>

#include <new>

#include "types.hpp"

namespace Brufs {

template <typename T>
class Vector {
private:
    Size size;
    Size capacity;

    union {
        T *ptr;
        void *vptr;
    };

public:
    template <typename... V>
    static inline Vector<T> of(V... values) {
        Vector<T> vec;
        vec.push_back(values...);
        return vec;
    }

    Vector(const unsigned int capacity = 8) :
        size(0), capacity(capacity), vptr(malloc(capacity * sizeof(T)))
    {
        assert(this->vptr != nullptr);
    }

    Vector(const Vector<T> &other) :
        size(other.size), capacity(other.capacity), vptr(malloc(other.capacity * sizeof(T)))
    {
        assert(this->vptr != nullptr);

        for (Size i = 0; i < this->size; ++i) {
            new (this->ptr + i) T(other[i]);
        }
    }

    template <typename I>
    Vector(I first, I last) : Vector() {
        for (I ptr = first; !(ptr == last); ++ptr) this->push_back(*ptr);
    }

    ~Vector() {
        this->clear();
        free(this->vptr);
    }

    Vector<T> &operator=(const Vector<T> &other) {
        this->size = other.size;
        this->capacity = other.capacity;

        this->vptr = realloc(this->vptr, other.capacity * sizeof(T));
        assert(this->vptr != nullptr);

        for (Size i = 0; i < this->size; ++i) {
            new (this->ptr + i) T(other[i]);
        }

        return *this;
    }

    T &operator[](Size pos) {
        return this->ptr[pos];
    }

    const T &operator[](Size pos) const {
        return this->ptr[pos];
    }

    T *data() {
        return this->ptr;
    }

    const T *data() const {
        return this->ptr;
    }

    T *begin() {
        return this->ptr;
    }

    const T *begin() const {
        return this->ptr;
    }

    T *end() {
        return this->ptr + this->size;
    }

    const T *end() const {
        return this->ptr + this->size;
    }

    T &front() {
        return *this->begin();
    }

    const T &front() const {
        return *this->begin();
    }

    T &back() {
        return this->ptr[this->size - 1];
    }

    const T &back() const {
        return this->ptr[this->size - 1];
    }

    Size get_size() const {
        return this->size;
    }

    Size get_capacity() const {
        return this->capacity;
    }

    void reserve(const Size req_cap) {
        if (req_cap < this->capacity) return;

        while (this->capacity < req_cap) this->capacity *= 1.4;

        this->vptr = realloc(this->vptr, this->capacity * sizeof(T));
        assert(this->vptr != nullptr);
    }

    void resize(Size new_size) {
        this->reserve(new_size);
        this->size = new_size;
    }

    void push_back(const T &value) {
        this->reserve(this->size + 1);
        new (this->ptr + this->size) T(value);
        ++this->size;
    }

    template <typename... V>
    void push_back(const T &value, const V... values) {
        this->push_back(value);
        this->push_back(values...);
    }

    void pop_back() {
        this->ptr[this->size - 1].~T();
        --this->size;
    }

    void clear() {
        while (this->size != 0) this->pop_back();
    }

    bool contains(const T &value) const {
        for (Size i = 0; i < this->size; ++i) {
            if (this->ptr[i] == value) return true;
        }

        return false;
    }

    bool empty() const {
        return this->size == 0;
    }

    bool operator==(const Vector<T> &other) const {
        if (this->size != other.size) return false;

        for (Size i = 0; i < this->size; ++i) {
            if (this->ptr[i] != other.ptr[i]) return false;
        }

        return true;
    }
};

}
