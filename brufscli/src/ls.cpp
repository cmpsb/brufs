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

#include <cstdio>

#include <string>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

#include "libbrufs.hpp"

#include "Util.hpp"
#include "FdAbst.hpp"

int ls(int argc, char **argv) {
    if (argc < 3) {
        fprintf(stderr, "Specify a file and a path.\n");
        return 3;
    }

    int iofd = open(argv[1], O_RDWR);
    if (iofd == -1) {
        fprintf(stderr, "Unable to open %s: %s", argv[1], strerror(errno));
        return 1;
    }

    FdAbst io(iofd);
    Brufs::Disk disk(&io);
    Brufs::Brufs fs(&disk);

    Brufs::Status status = fs.get_status();
    if (status < Brufs::Status::OK) {
        fprintf(stderr, "%s\n", Brufs::strerror(status));
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
            root_name.c_str(), Brufs::strerror(status)
        );
        return 1;
    }

    Brufs::Root root(fs, root_header);

    Brufs::Directory dir(root);
    status = root.open_directory(Brufs::ROOT_DIR_INODE_ID, dir);
    if (status < Brufs::Status::OK) {
        fprintf(stderr, "Unable to open root directory %s: %s\n",
            root_name.c_str(), Brufs::strerror(status)
        );

        return 1;
    }

    Brufs::String local_path = path.substr(
        colon_pos + 1, 
        path.back() == '/' ? 
            path.get_size() - colon_pos - 2 :
            Brufs::String::npos
    );

    if (local_path.front() == '/') local_path = local_path.substr(1);

    while (!local_path.empty()) {
        auto slash_pos = local_path.find('/');
        Brufs::String component = local_path.substr(0, slash_pos);
        local_path = slash_pos != Brufs::String::npos ? local_path.substr(slash_pos + 1) : "";

        Brufs::DirectoryEntry entry;
        status = dir.look_up(component.c_str(), entry);
        if (status < Brufs::Status::OK) {
            fprintf(stderr, "Unable to locate %s: %s\n", 
                component.c_str(), Brufs::strerror(status)
            );

            return 1;
        }

        Brufs::Directory subdir(root);
        status = root.open_directory(entry.inode_id, subdir);
        if (status < Brufs::Status::OK) {
            fprintf(stderr, "Unable to open %s as a directory: %s\n", 
                component.c_str(), Brufs::strerror(status)
            );

            return 1;
        }

        dir = subdir;
    }

    Brufs::Vector<Brufs::DirectoryEntry> entries;
    status = dir.collect(entries);
    if (status < Brufs::Status::OK) {
        fprintf(stderr, "Unable to collect the directory's entries: %s\n",
            Brufs::strerror(status)
        );

        return 1;
    }

    auto hdr = root.create_inode_header();

    for (const auto &entry : entries) {
        char label[Brufs::MAX_LABEL_LENGTH + 1];
        memcpy(label, entry.label, Brufs::MAX_LABEL_LENGTH);
        label[Brufs::MAX_LABEL_LENGTH] = 0;

        Brufs::String inode_str = Util::pretty_print_inode_id(entry.inode_id);

        status = root.find_inode(entry.inode_id, hdr);
        if (status < Brufs::Status::OK) {
            fprintf(stderr, "Unable to load inode %s: %s", inode_str.c_str(), io.strstatus(status));
            continue;
        }

        Brufs::String mtime_str = Util::pretty_print_timestamp(hdr->last_modified);

        Brufs::String mode_str = Util::pretty_print_mode(
            hdr->type == Brufs::InodeType::DIRECTORY,
            hdr->mode
        );

        printf("%s %s %s\n", mode_str.c_str(), mtime_str.c_str(), label);
    }

    return 0;
}
