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
#include <cstring>

#include "Vector.hpp"

namespace Brufs {

class String : private Vector<char> {
private:
    void terminate() {
        Vector::back() = 0;
    }

public:
    static constexpr Offset npos = -1;

    String() : String("") {}

    String(const char *str) : String(str, strlen(str)) {}

    String(const char *str, Size length) {
        this->resize(length + 1);
        memcpy(this->begin(), str, length);
        this->terminate();
    }

    String(const String &other) : Vector(other) {}

    using Vector::operator=;
    using Vector::operator[];
    using Vector::front;
    using Vector::begin;

    const char *c_str() const {
        return this->data();
    }

    operator const char *() const {
        return this->c_str();
    }

    operator char *() {
        return this->data();
    }

    const char &back() const {
        return this->c_str()[this->size() - 1];
    }

    char &back() {
        return this->data()[this->size() - 1];
    }

    const char *end() const {
        return Vector::end() - 1;
    }

    char *end() {
        return Vector::end() - 1;
    }

    Size size() const {
        return Vector::get_size() - 1;
    }

    Size length() const {
        return this->size();
    }

    String operator+(const String &other) const {
        String copy(*this);

        copy.resize(this->size() + other.size() + 1);
        memcpy(copy.data() + this->size(), other.data(), other.get_size());
        copy.terminate();

        return copy;
    }

    bool empty() const {
        return this->size() == 0;
    }

    Offset find(char ch, Offset pos = 0) const {
        auto ptr = strchr(this->c_str() + pos, ch);
        if (ptr == nullptr) return npos;

        return static_cast<Offset>(ptr - this->c_str());
    }

    String substr(Offset pos, Size count = npos) const {
        auto remaining_size = this->size() - pos;
        auto actual_count = count == npos ? remaining_size : count;
        auto actual_size = actual_count < remaining_size ? actual_count : remaining_size;

        String substring;
        substring.resize(actual_size + 1);
        memcpy(substring.data(), this->c_str() + pos, actual_size);
        substring.terminate();

        return substring;
    }

    Vector<String> split(const char splch) const {
        Vector<String> portions;

        Offset start = 0;

        for (;;) {
            auto choff = this->find(splch, start);
            if (choff == npos) {
                portions.push_back(this->substr(start));
                break;
            }

            portions.push_back(this->substr(start, choff - start));
            start = choff + 1;
        }

        return portions;
    }

    bool operator==(const String &other) const {
        if (this->size() != other.size()) return false;

        return memcmp(this->c_str(), other.c_str(), this->size()) == 0;
    }

    bool operator==(const char *str) const {
        return *this == String(str);
    }

    bool operator!=(const String &other) const {
        return !(*this == other);
    }
};

}
