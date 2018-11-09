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

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

#include "libbrufs.hpp"

#include "FdAbst.hpp"

static constexpr unsigned int TRANSFER_BUFFER_SIZE = 128 * 1024 * 1024;

int copy_in(int argc, char **argv) {
    if (argc < 4) {
        fprintf(stderr, "Specify a file, a path, and a file to copy to that location.\n");
        return 3;
    }

    FILE *in_file = fopen(argv[3], "rb");
    if (!in_file) {
        fprintf(stderr, "Unable to open %s: %s\n", argv[3], strerror(errno));
        return 1;
    }

    int iofd = open(argv[1], O_RDWR);
    if (iofd == -1) {
        fprintf(stderr, "Unable to open %s: %s\n", argv[1], strerror(errno));
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

    const Brufs::String path = argv[2];
    const auto colon_pos = path.find(':');
    if (colon_pos == Brufs::String::npos) {
        fprintf(stderr, "The path does not contain a root.\n");
        return 1;
    }

    const auto root_name = path.substr(0, colon_pos);
    Brufs::RootHeader root_header;
    status = fs.find_root(root_name.c_str(), root_header);
    if (status < Brufs::Status::OK) {
        fprintf(stderr, "Unable to find root %s: %s\n",
            root_name.c_str(), io.strstatus(status)
        );
        return 1;
    }

    Brufs::Root root(fs, root_header);

    Brufs::Directory dir(root);
    status = root.open_directory(Brufs::ROOT_DIR_INODE_ID, dir);
    if (status < Brufs::Status::OK) {
        fprintf(stderr, "Unable to open root directory %s: %s\n",
            root_name.c_str(), io.strstatus(status)
        );
    }

    Brufs::String local_path = path.substr(
        colon_pos + 1,
        path.back() == '/' ?
            path.size() - colon_pos - 2 :
            Brufs::String::npos
    );

    if (local_path.front() == '/') local_path = local_path.substr(1);

    while (local_path.find('/') != Brufs::String::npos) {
        auto slash_pos = local_path.find('/');
        Brufs::String component = local_path.substr(0, slash_pos);
        local_path = slash_pos != Brufs::String::npos ? local_path.substr(slash_pos + 1) : "";

        Brufs::DirectoryEntry entry;
        status = dir.look_up(component.c_str(), entry);
        if (status < Brufs::Status::OK) {
            fprintf(stderr, "Unable to locate %s: %s\n",
                component.c_str(), io.strstatus(status)
            );

            return 1;
        }

        Brufs::Directory subdir(root);
        status = root.open_directory(entry.inode_id, subdir);
        if (status < Brufs::Status::OK) {
            fprintf(stderr, "Unable to open %s as a directory: %s\n",
                component.c_str(), io.strstatus(status)
            );

            return 1;
        }

        dir = subdir;
    }

    Brufs::DirectoryEntry entry;
    status = dir.look_up(local_path, entry);
    if (status < Brufs::Status::OK) {
        fprintf(stderr, "Unable to locate %s to write to: %s\n",
            local_path.c_str(), io.strstatus(status)
        );

        return 1;
    }

    auto hdr = root.create_inode_header();
    status = root.find_inode(entry.inode_id, hdr);
    if (status < Brufs::Status::OK) {
        fprintf(stderr, "Unable to open %s for writing: %s\n",
            local_path.c_str(), io.strstatus(status)
        );

        return 1;
    }

    Brufs::File file(root);
    file.init(entry.inode_id, hdr);
    root.destroy_inode_header(hdr);

    auto buf = new char[TRANSFER_BUFFER_SIZE];
    assert(buf);
    Brufs::Offset offset = 0;

    for (;;) {
        auto num_read = fread(buf, 1, TRANSFER_BUFFER_SIZE, in_file);
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
            auto num_written = file.write(
                buf + num_transferred, num_read - num_transferred, offset + num_transferred
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

    printf("Copied %llu bytes: %s\n", offset, io.strstatus(status < 0 ? status : 0));

    return 0;
}
