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

#include <cstdio>
#include <cerrno>

#include <vector>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

#include "libbrufs.hpp"

#include "FdAbst.hpp"

static constexpr unsigned int TRANSFER_BUFFER_SIZE = 128 * 1024 * 1024;

int copy_in(int argc, char **argv) {
    if (argc != 3) {
        fprintf(stderr, "Specify a path and a file to copy to that location.\n");
        return 3;
    }

    Brufs::PathParser path_parser;
    auto path = path_parser.parse(argv[1]);

    if (!path.has_partition()) {
        fprintf(stderr,
            "The path does not contain a partition (file, block device, ...) where the "
            "filesystem is stored."
        );
        return 1;
    }

    if (!path.has_root()) {
        fprintf(stderr, "The path does not contain a root.\n");
        return 1;
    }

    FILE *in_file = fopen(argv[2], "rb");
    if (!in_file) {
        fprintf(stderr, "Unable to open %s: %s\n", argv[2], strerror(errno));
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

    const auto root_name = path.get_root();
    Brufs::RootHeader root_header;
    status = fs.find_root(root_name.c_str(), root_header);
    if (status < Brufs::Status::OK) {
        fprintf(stderr, "Unable to find root %s: %s\n",
            root_name.c_str(), io.strstatus(status)
        );
        return 1;
    }

    Brufs::Root root(fs, root_header);

    Brufs::File file(root);
    status = root.open_file(path, file);
    if (status < Brufs::Status::OK) {
        fprintf(stderr, "Unable to open file %s for writing: %s\n",
            argv[1], io.strstatus(status)
        );
        return 1;
    }

    Brufs::Vector<char> buf(TRANSFER_BUFFER_SIZE);
    Brufs::Offset offset = 0;

    for (;;) {
        auto num_read = fread(buf.data(), 1, TRANSFER_BUFFER_SIZE, in_file);
        if (num_read == 0) {
            if (feof(in_file)) break;
            if (ferror(in_file)) {
                fprintf(stderr, "I/O error while reading from the source file: %s\n",
                    strerror(errno)
                );

                return 1;
            }

            assert("either eof or error, bad C library?" == nullptr);
        }

        Brufs::Size num_transferred = 0;
        while (num_transferred < num_read) {
            printf("w %p, %lu, %lu\n", buf.data() + num_transferred, num_read - num_transferred, offset + num_transferred);
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

    printf("Copied %llu bytes: %s\n", offset, io.strstatus(status));

    return 0;
}
