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

#include "String.hpp"

namespace Brufs {

/**
 * A path on the filesystem.
 *
 * May optionally include a partition and root name.
 */
class Path {
private:
    /**
     * The partition the path is on, or an empty string.
     */
    const String partition;

    /**
     * The root the path is on, or an empty string.
     */
    const String root;

    /**
     * The path components.
     */
    const Vector<String> components;

public:
    /**
     * Creates a new path from a partition, root and components.
     *
     * @param partition the partition the path is on
     * @param root the root the path is on
     * @param components the components of the path
     */
    Path(const String &partition, const String &root, const Vector<String> &components) :
        partition(partition), root(root), components(components)
    {}

    /**
     * Creates a new path from a root and components.
     *
     * The partition is left blank.
     *
     * @param root the root the path is on
     * @param components the components
     */
    Path(const String &root, const Vector<String> &components) : Path("", root, components) {}

    /**
     * Creates a new path from components.
     *
     * The partition and root are left blank.
     *
     * @param components the components
     */
    Path(const Vector<String> &components) : Path("", components) {}

    /**
     * Create a new path representing the path "/".
     *
     * The partition and root are left blank.
     */
    Path() : Path(Vector<String>(0)) {}

    /**
     * Creates a new path by copying another.
     *
     * @param other the path to copy
     */
    Path(const Path &other) : Path(other.partition, other.root, other.components) {}

    /**
     * Checks whether the path contains a partition.
     *
     * @return true if the path contains a partition, false otherwise
     */
    bool has_partition() const { return !this->partition.empty(); }

    /**
     * Returns the partition the path is on, or an empty string if it's unspecified.
     *
     * @return the partition
     */
    const String &get_partition() const { return this->partition; }

    /**
     * Checks whether the path contains a root.
     *
     * @return true if the path contains a root, false otherwise
     */
    bool has_root() const { return !this->root.empty(); }

    /**
     * Returns the root the partition is on, or an empty string if it's unspecified.
     *
     * @return the root
     */
    const String &get_root() const { return this->root; }

    /**
     * Returns the components of the path.
     *
     * @return the components
     */
    const Vector<String> &get_components() const { return this->components; }

    /**
     * Returns the parent of the path.
     *
     * If the path is at the root already, the path itself is returned.
     *
     * @return the parent of the path
     */
    Path get_parent() const {
        auto parent_components = this->components;
        if (parent_components.get_size() > 0) parent_components.pop_back();

        return Path(this->partition, this->root, parent_components);
    }

    /**
     * Concatenates another path to this path.
     *
     * The partition and root of the other path, if any, are ignored.
     *
     * @param other the path to concatenate
     *
     * @return the concatenated paths
     */
    Path resolve(const Path &other) const {
        auto child_components = this->components;
        for (const auto &component : other.components) child_components.push_back(component);

        return Path(this->partition, this->root, child_components);
    }

    bool operator==(const Path &other) const {
        return this->partition == other.partition
            && this->root == other.root
            && this->components == other.components;
    }

    bool operator!=(const Path &other) const {
        return !(*this == other);
    };

};

}
