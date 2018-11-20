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
#include <string>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

#include "libbrufs.hpp"

#include "FdAbst.hpp"
#include "PathValidator.hpp"

static constexpr unsigned long long int TRANSFER_BUFFER_SIZE = 128 * 1024 * 1024;

int copy_out(int argc, char **argv) {
    if (argc != 3) {
        fprintf(stderr, "Specify a file, a path, and a file to copy to that location.\n");
        return 3;
    }

    Brufs::PathParser path_parser;
    auto path = path_parser.parse(argv[1]);

    Brufscli::PathValidator validator;
    validator.validate(path, true, true);

    std::string out_path = "-";
    if (argc == 3) out_path = argv[2];

    FILE *out_file = out_path == "-" ? stdout : fopen(out_path.c_str(), "wb");
    if (!out_file) {
        fprintf(stderr, "Unable to open %s: %s\n", out_path.c_str(), strerror(errno));
        return 1;
    }

    const auto disk_path = path.get_partition().c_str();
    int iofd = open(disk_path, O_RDWR);
    if (iofd == -1) {
        fprintf(stderr, "Unable to open %s: %s\n", disk_path, strerror(errno));
        return 1;
    }

    FdAbst io{iofd};
    Brufs::Disk disk(&io);
    Brufs::Brufs fs(&disk);

    Brufs::Status status = fs.get_status();
    if (status < Brufs::Status::OK) {
        fprintf(stderr, "%s\n", io.strstatus(status));
        return 1;
    }

    const auto root_name = path.get_root().c_str();
    Brufs::RootHeader root_header;
    status = fs.find_root(root_name, root_header);
    if (status < Brufs::Status::OK) {
        fprintf(stderr, "Unable to find root %s: %s\n",
            root_name, io.strstatus(status)
        );
        return 1;
    }

    Brufs::Root root(fs, root_header);

    Brufs::File file(root);
    status = root.open_file(path, file);
    if (status < Brufs::Status::OK) {
        fprintf(stderr, "Unable to open %s for writing: %s\n",
            argv[1], io.strstatus(status)
        );

        return 1;
    }

    const auto size = file.get_size();
    Brufs::Vector<char> buf(TRANSFER_BUFFER_SIZE);
    Brufs::Offset offset = 0;

    while (offset < size) {
        auto to_read = std::min(TRANSFER_BUFFER_SIZE, size - offset);
        auto num_read = file.read(buf.data(), to_read, offset);
        if (num_read < Brufs::Status::OK) {
            fprintf(stderr, "Unable to read %lld bytes: %s\n", to_read, io.strstatus(status));
            return 1;
        }

        Brufs::SSize num_transferred = 0;
        while (num_transferred < num_read) {
            auto num_written = fwrite(
                buf.data() + num_transferred, 1, num_read - num_transferred, out_file
            );
            if (num_written == 0) {
                if (feof(out_file)) break;
                if (ferror(out_file)) {
                    fprintf(stderr, "I/O error while writing to the target file: %s\n",
                        strerror(errno)
                    );

                    return 1;
                }

                assert("neither eof or error, bad C library?" == nullptr);
            }

            num_transferred += num_written;
        }

        offset += num_transferred;
    }

    fflush(out_file);

    return 0;
}
