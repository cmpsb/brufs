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

#include "VersionAction.hpp"

std::vector<std::string> Brufscli::VersionAction::get_names() const {
    return {"version"};
}

int Brufscli::VersionAction::run([[maybe_unused]] const std::string &name) {
    const auto build_info = Brufs::BuildInfo::get();

    constexpr unsigned int VERSION_BUFFER_LENGTH = 64;
    char version[VERSION_BUFFER_LENGTH];
    build_info.version.to_string(version, VERSION_BUFFER_LENGTH);

    const char *release_type;
    const auto is_debug = build_info.is_debug();
    const auto is_release = build_info.is_release();
    if (is_debug && !is_release) release_type = "debug";
    if (!is_debug && is_release) release_type = "release";
    if (is_debug && is_release) release_type = "invalid build type: both debug and release";
    if (!is_debug && !is_release) release_type = "invalid build type: neither debug nor release";

    this->logger.info("Brufs v%s (%s)", version, release_type);
    this->logger.info("Built %s", build_info.build_date.c_str());

    const auto is_from_git = build_info.is_from_git();
    this->logger.info("git: %s", is_from_git ? "yes" : "no");
    if (is_from_git) {
        this->logger.info("  tag: %s", build_info.git_tag.c_str());
        this->logger.info("  branch: %s", build_info.git_branch.c_str());
        this->logger.info("  commit: %s %s",
            build_info.git_commit.c_str(), build_info.is_dirty() ? "(DIRTY)" : "(clean)"
        );
    }

    return 0;
}
