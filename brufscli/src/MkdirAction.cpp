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

#include <cstdio>
#include <cerrno>
#include <cstring>
#include <ctime>

#include "MkdirAction.hpp"
#include "Util.hpp"

std::vector<slopt_Option> Brufscli::MkdirAction::get_options() const {
    return {
        {'p', "parents", SLOPT_DISALLOW_ARGUMENT}
    };
}

std::vector<std::string> Brufscli::MkdirAction::get_names() const {
    return {"mkdir"};
}

void Brufscli::MkdirAction::apply_option(
    int sw,
    int snam, [[maybe_unused]] const std::string &lnam,
    const std::string &val
) {
    if (sw == SLOPT_DIRECT && this->spec.empty()) {
        this->spec = val;
        return;
    }

    if (sw == SLOPT_DIRECT) {
        throw InvalidArgumentException(
            "Unexpected value " + val + " (target is " + this->spec + ")"
        );
    }

    switch (snam) {
    case 'p':
        this->create_parents = true;
        break;

    case 'm':
        this->inode_header_builder.with_mode(std::stoi(val, 0, 8));
        break;

    case 'o':
        this->inode_header_builder.with_owner(std::stoul(val));
        break;

    case 'g':
        this->inode_header_builder.with_group(std::stoul(val));
        break;
    }

}

void Brufscli::MkdirAction::mkdir(
    const Brufs::AbstIO &io, Brufs::Root &root, Brufs::Path path
) const {
    Brufs::Directory dir(root);
    auto status = this->creator.create_directory(path, this->inode_header_builder, dir);

    if (status == Brufs::Status::E_NOT_FOUND && this->create_parents) {
        auto sub_path = path.get_parent();

        this->mkdir(io, root, sub_path);

        auto sub_path_str = this->path_parser.unparse(sub_path);
        this->logger.info("%s", sub_path_str.c_str());

        status = this->creator.create_directory(path, this->inode_header_builder, dir);
        this->on_error(status,
            std::string("Unable to create parent ") + sub_path_str.c_str() + ": ", io
        );
    }

    this->on_error(status, "Unable to create the directory: ", io);
}

int Brufscli::MkdirAction::run([[maybe_unused]] const std::string &name) {
    auto path = this->path_parser.parse({this->spec.c_str(), this->spec.length()});
    this->path_validator.validate(path, true, true);

    auto brufs = this->opener.open_existing(path.get_partition());
    auto &fs = brufs.get_fs();
    const auto &io = brufs.get_io();

    const auto root_name = path.get_root();
    Brufs::RootHeader root_header;
    auto status = fs.find_root(root_name.c_str(), root_header);
    this->on_error(status, std::string("Unable to open root ") + root_name.c_str() + ": ", io);

    Brufs::Root root(fs, root_header);
    this->mkdir(io, root, path);

    return 0;
}
