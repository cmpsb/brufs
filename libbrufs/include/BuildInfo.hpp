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

#include "Version.hpp"
#include "Vector.hpp"
#include "String.hpp"

namespace Brufs {

/**
 * A struct describing the library's build information, such as the version and any
 * compile-time configuration flags.
 */
struct BuildInfo {
    /**
     * The version of the library.
     */
    Version version;

    /**
     * Any compile-time flags, including the build type and whether the build was executed from
     * a git repository.
     */
    Vector<String> flags;

    /**
     * The datetime the library was built.
     */
    String build_date;

    /**
     * The name of the most recent git tag.
     * If the build was not executed from a git repository, this contains an empty string.
     */
    String git_tag;

    /**
     * The name of the git branch.
     * If the build was not executed from a git repository, this contains an empty string.
     */
    String git_branch;

    /**
     * The hash of the most recent git commit.
     * If the build was not executed from a git repository, this contains an empty string.
     */
    String git_commit;

    bool is_from_git() const { return this->flags.contains("git"); }
    bool is_dirty() const { return this->flags.contains("dirty"); }
    bool is_debug() const { return this->flags.contains("debug"); }
    bool is_release() const { return this->flags.contains("release"); }

    /**
     * Returns the compile-time build info of the library.
     *
     * @return the info
     */
    static BuildInfo get();
};

}
