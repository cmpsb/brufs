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

#include "PathParser.hpp"
#include "StringBuilder.hpp"

Brufs::Path Brufs::PathParser::parse(const String &path) const {
    String partition = "";
    String root = "";
    Vector<String> components;

    Offset last_colon = String::npos;
    if (auto first_colon = path.find(':'); first_colon != String::npos) {
        if (auto second_colon = path.find(':', first_colon + 1); second_colon != String::npos) {
            partition = path.substr(0, first_colon);
            root = path.substr(first_colon + 1, second_colon - first_colon - 1);

            last_colon = second_colon;
        } else {
            root = path.substr(0, first_colon);

            last_colon = first_colon;
        }
    }

    String local_path = path.substr(
        path[last_colon + 1] == '/' ? last_colon + 2 : last_colon + 1,
        path.back() == '/' ? path.size() - last_colon - 2 : String::npos
    );

    Offset slash_pos = 0;
    while ((slash_pos = local_path.find('/')) != String::npos) {
        components.push_back(local_path.substr(0, slash_pos));
        while (local_path[slash_pos] == '/') ++slash_pos;

        local_path = local_path.substr(slash_pos);
    }

    if (!local_path.empty()) components.push_back(local_path);

    return {partition, root, components};
}

Brufs::String Brufs::PathParser::unparse(const Path &path) const {
    StringBuilder builder;

    // A partition but no root is malformed, so only include the partition if there's a root as well
    if (path.has_partition() && path.has_root()) {
        builder.append(path.get_partition()).append(':');
    }

    if (path.has_root()) {
        builder.append(path.get_root()).append(':');
    }

    if (path.get_components().empty()) builder.append('/');

    for (const auto &component : path.get_components()) {
        builder.append('/');
        builder.append(component);
    }

    return builder;
}
