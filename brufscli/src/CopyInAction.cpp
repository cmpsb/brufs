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

#include "CopyInAction.hpp"
#include "Util.hpp"

std::vector<slopt_Option> Brufscli::CopyInAction::get_options() const {
    return {
        {'b', "buffer", SLOPT_REQUIRE_ARGUMENT},
        {'c', "create", SLOPT_DISALLOW_ARGUMENT},
        {'m', "mode", SLOPT_REQUIRE_ARGUMENT},
        {'u', "owner", SLOPT_REQUIRE_ARGUMENT},
        {'g', "group", SLOPT_REQUIRE_ARGUMENT}
    };
}

std::vector<std::string> Brufscli::CopyInAction::get_names() const {
    return {"copy-in", "copyin", "write"};
}

void Brufscli::CopyInAction::apply_option(
    int sw,
    [[maybe_unused]] int snam, [[maybe_unused]] const std::string &lnam,
    const std::string &val
) {
    if (sw == SLOPT_DIRECT && this->spec.empty()) {
        this->spec = val;
        return;
    }

    if (sw == SLOPT_DIRECT && this->in_path.empty()) {
        this->in_path = val;
        return;
    }

    if (sw == SLOPT_DIRECT) {
        throw InvalidArgumentException(
            "Unexpected value " + val
            + " (target is " + this->spec + ", source is " + this->in_path + ")"
        );
    }

    switch (snam) {
    case 'b':
        this->transfer_buffer_size = std::stoul(val);
        break;

    case 'c':
        this->create = true;
        break;

    case 'm':
        this->inode_header_builder.with_mode(std::stoi(val, 0, 8));
        break;

    case 'u':
        this->inode_header_builder.with_owner(std::stoul(val));
        break;

    case 'g':
        this->inode_header_builder.with_group(std::stoul(val));
        break;
    }
}

int Brufscli::CopyInAction::run([[maybe_unused]] const std::string &name) {
    auto path = this->path_parser.parse({this->spec.c_str(), this->spec.length()});
    this->path_validator.validate(path, true, true);

    auto brufs = this->opener.open_existing(path.get_partition());
    auto &fs = brufs.get_fs();
    const auto &io = brufs.get_io();

    if (this->in_path.empty()) this->in_path = "-";

    auto in_file = (this->in_path == "-") ? stdin : fopen(this->in_path.c_str(), "rb");
    if (!in_file) {
        throw std::runtime_error("Unable to open " + this->in_path + ": " + strerror(errno));
    }

    const auto root_name = path.get_root();
    Brufs::RootHeader root_header;
    auto status = fs.find_root(root_name.c_str(), root_header);
    this->on_error(status, std::string("Unable to open root ") + root_name.c_str() + ": ", io);

    Brufs::Root root(fs, root_header);

    Brufs::File file(root);
    status = root.open_file(path, file);
    if (status == Brufs::Status::E_NOT_FOUND && this->create) {
        status = this->creator.create_file(path, this->inode_header_builder, file);
        this->on_error(status, "Unable to create the file: ", io);
    }
    this->on_error(status, "Unable to open the file for writing: ", io);

    std::vector<char> buf(this->transfer_buffer_size);
    Brufs::Offset offset = 0;

    for (;;) {
        auto num_read = fread(buf.data(), 1, this->transfer_buffer_size, in_file);
        if (num_read == 0) {
            if (feof(in_file)) break;
            if (ferror(in_file)) {
                throw std::runtime_error(
                    std::string("I/O error while reading from the source file: ") + strerror(errno)
                );
            }

            assert("either eof or error, bad C library?" == nullptr);
        }

        Brufs::Size num_transferred = 0;
        while (num_transferred < num_read) {
            auto num_written = file.write(
                buf.data() + num_transferred, num_read - num_transferred, offset + num_transferred
            );

            if (num_written < Brufs::Status::OK) {
                fprintf(stderr, "Unable to write %lu bytes: %s\n",
                    num_read - num_transferred, io.strstatus(num_written)
                );

                return 1;
            }

            num_transferred += num_written;
        }

        offset += num_transferred;
    }

    this->logger.debug("Copied %llu bytes", offset);

    return 0;
}
