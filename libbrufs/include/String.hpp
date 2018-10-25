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

#include <cstring>

#include "Vector.hpp"

namespace Brufs {

class String : public Vector<char> {
private:
    void terminate() {
        this->back() = 0;
    }

public:
    static constexpr Offset npos = -1;

    String() {}

    String(const char *str) : String(str, strlen(str)) {}

    String(const char *str, Size length) {
        this->resize(length + 1);
        memcpy(this->begin(), str, length);
        this->terminate();
    }

    String(const String &other) : Vector(other) {}

    String &operator=(const String &other) {
        Vector::operator=(other);
        return *this;
    }

    const char *c_str() const {
        return this->data();
    }

    operator const char *() const {
        return this->c_str();
    }

    operator char *() {
        return this->data();
    }

    Size get_size() const {
        return Vector::get_size() - 1;
    }

    String operator+(const String &other) {
        String copy(*this);

        copy.resize(this->get_size() + other.get_size() + 1);
        memcpy(copy.data() + this->get_size(), other.data(), other.get_size());
        copy.terminate();

        return copy;
    }

    bool empty() const {
        return this->get_size() == 0;
    }

    Offset find(char ch) const {
        auto ptr = strchr(this->c_str(), ch);
        if (ptr == nullptr) return npos;

        return static_cast<Offset>(ptr - this->c_str());
    }

    String substr(Offset pos, Size count = npos) const {
        auto remaining_size = this->get_size() - pos;
        auto actual_count = count == npos ? remaining_size : count;
        auto actual_size = actual_count < remaining_size ? actual_count : remaining_size;

        String substring;
        substring.resize(actual_size + 1);
        memcpy(substring.data(), this->c_str() + pos, actual_size);
        substring.terminate();

        return substring;
    }

    void fit() {
        this->terminate();
        this->resize(strlen(this->c_str()) + 1);
    }
};

}
