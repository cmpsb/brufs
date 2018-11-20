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

#include <stdexcept>

#include "libbrufs.hpp"

namespace Brufscli {

class PathValidationException : public std::runtime_error {
public:
    using std::runtime_error::runtime_error;
};

class NoPartitionException : public PathValidationException {
public:
    using PathValidationException::PathValidationException;
};

class NoRootException : public PathValidationException {
public:
    using PathValidationException::PathValidationException;
};

/**
 * A validator for user-generated paths.
 */
class PathValidator {
public:
    /**
     * Validates a given path.
     *
     * @param path the path to validate
     * @param require_partition if true, assert that the path contains a partition
     * @param require_root if true, assert that the path contains a root
     *
     * @throws NoPartitionException if require_partition is true and the path has no partition
     * @throws NoRootException if require_root is true and the path has no root
     */
    virtual void validate(
        const Brufs::Path &path,
        bool require_partition = true,
        bool require_root = true
    );
};

}
