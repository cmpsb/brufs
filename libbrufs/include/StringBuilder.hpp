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
 * OUT OF OR IN CONNECTION WITH THE OSFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#pragma once

#include <string.h>

#include "String.hpp"

namespace Brufs {

/**
 * A class for building strings by continuously appending new data.
 */
class StringBuilder {
private:
    /**
     * The buffer containing the chars representing the new string.
     */
    Vector<char> str;
public:
    /**
     * Creates a string builder.
     *
     * The initial string is empty.
     */
    StringBuilder() {}

    /**
     * Creates a string builder by copying another.
     *
     * @param other the builder to copy
     */
    StringBuilder(const StringBuilder &other) : str(other.str) {}

    /**
     * Creates a string builder by starting with an existing string.
     *
     * @param basis the initial string
     */
    StringBuilder(const String &basis) {
        this->append(basis);
    }

    StringBuilder &operator=(const StringBuilder &other) {
        this->str = other.str;
        return *this;
    }

    StringBuilder &operator=(const String &str) {
        this->str.clear();
        return this->append(str);
    }

    StringBuilder &operator=(char ch) {
        this->str.clear();
        return this->append(ch);
    }

    /**
     * Appends a string.
     *
     * @param str the string to append
     *
     * @return the string builder
     */
    StringBuilder &append(const String &str) {
        auto old_size = this->str.get_size();

        this->str.resize(this->str.get_size() + str.size());
        memcpy(this->str.data() + old_size, str.c_str(), str.size());

        return *this;
    }

    /**
     * Appends a character.
     *
     * @param ch the character to append
     * @return the string builder
     */
    StringBuilder &append(char ch) {
        this->str.push_back(ch);

        return *this;
    }

    /**
     * Appends a string.
     *
     * @param str the string to append
     */
    StringBuilder &operator+=(const String &str) {
        return this->append(str);
    }

    /**
     * Appends a string.
     *
     * @param str the string to append
     *
     * @return the string builder with the string appended
     */
    StringBuilder operator+(const String &str) const {
        StringBuilder copy(*this);
        copy += str;
        return copy;
    }

    /**
     * Appends a character.
     *
     * @param ch the character to append
     */
    StringBuilder &operator+=(char ch) {
        return this->append(ch);
    }

    /**
     * Appends a character.
     *
     * @param ch the character to append
     *
     * @return the string builder with the character appended
     */
    StringBuilder operator+(char ch) const {
        StringBuilder copy(*this);
        copy += ch;
        return copy;
    }

    /**
     * Extracts the currently-built string.
     *
     * The returned value contains a copy of the current state, so subsequent modifications to this
     * builder will not reflect in the returned string.
     *
     * @return the string
     */
    String to_string() const {
        return {this->str.data(), this->str.get_size()};
    }

    /**
     * Implicitly converts the builder into a string.
     *
     * Follows the same semantics as #to_string().
     *
     * @return the string
     */
    operator String() const {
        return this->to_string();
    }
};

}
