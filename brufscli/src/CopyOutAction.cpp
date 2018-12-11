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

#include "CopyOutAction.hpp"
#include "Util.hpp"


std::vector<slopt_Option> Brufscli::CopyOutAction::get_options() const {
    return {
        {'b', "buffer", SLOPT_REQUIRE_ARGUMENT}
    };
}

std::vector<std::string> Brufscli::CopyOutAction::get_names() const {
    return {"copy-out", "copyout", "read", "cat"};
}

void Brufscli::CopyOutAction::apply_option(
    int sw,
    int snam, [[maybe_unused]] const std::string &lnam,
    const std::string &val
) {
    if (sw == SLOPT_DIRECT && this->spec.empty()) {
        this->spec = val;
        return;
    }

    if (sw == SLOPT_DIRECT && this->out_path.empty()) {
        this->out_path = val;
        return;
    }

    if (sw == SLOPT_DIRECT) {
        throw InvalidArgumentException(
            "Unexpected value " + val
            + " (target is " + this->spec + ", source is " + this->out_path + ")"
        );
    }

    switch (snam) {
    case 'b':
        this->transfer_buffer_size = std::stoul(val);
        break;
    }
}

int Brufscli::CopyOutAction::run([[maybe_unused]] const std::string &name) {
    auto path = this->path_parser.parse({this->spec.c_str(), this->spec.length()});
    this->path_validator.validate(path, true, true);

    auto brufs = this->opener.open_existing(path.get_partition());
    auto &fs = brufs.get_fs();
    const auto &io = brufs.get_io();

    if (this->out_path.empty()) this->out_path = "-";

    auto out_file = (this->out_path == "-") ? stdout : fopen(this->out_path.c_str(), "wb");
    if (!out_file) {
        throw std::runtime_error("Unable to open " + this->out_path + ": " + strerror(errno));
    }

    const auto root_name = path.get_root();
    Brufs::RootHeader root_header;
    auto status = fs.find_root(root_name.c_str(), root_header);
    this->on_error(status, std::string("Unable to open root ") + root_name.c_str() + ": ", io);

    Brufs::Root root(fs, root_header);

    Brufs::File file(root);
    status = root.open_file(path, file);
    this->on_error(status, "Unable to open the file for reading: ", io);

    const auto size = file.get_size();
    Brufs::Vector<char> buf(this->transfer_buffer_size);
    Brufs::Offset offset = 0;

    while (offset < size) {
        auto to_read = std::min(this->transfer_buffer_size, static_cast<size_t>(size - offset));
        auto num_read = file.read(buf.data(), to_read, offset);
        this->on_error(static_cast<Brufs::Status>(num_read),
            "Unable to read " + std::to_string(to_read) + " bytes: ", io
        );

        Brufs::SSize num_transferred = 0;
        while (num_transferred < num_read) {
            auto num_written = fwrite(
                buf.data() + num_transferred, 1, num_read - num_transferred, out_file
            );
            if (num_written == 0) {
                if (feof(out_file)) break;
                if (ferror(out_file)) {
                    throw std::runtime_error(
                        std::string("I/O error while writing to the target file: ")
                        + strerror(errno)
                    );
                }

                assert("neither eof or error, bad C library?" == nullptr);
            }

            num_transferred += num_written;
        }

        offset += num_transferred;
    }

    fflush(out_file);

    this->logger.debug("Copied %llu bytes", offset);

    return 0;
}
