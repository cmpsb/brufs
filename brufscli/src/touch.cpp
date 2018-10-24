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

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

#include "libbrufs.hpp"

#include "FdAbst.hpp"
#include "Util.hpp"

int touch(int argc, char **argv) {
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
            path.get_size() - colon_pos - 2 :
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

    mode_t mode_mask = umask(0);
    umask(mode_mask);

    Brufs::File new_file(root);

    auto rdh = new_file.get_header();
    rdh->created = Brufs::Timestamp::now();
    rdh->last_modified = Brufs::Timestamp::now();
    rdh->owner = geteuid();
    rdh->group = getegid();
    rdh->num_links = 1;
    rdh->type = Brufs::InodeType::FILE;
    rdh->flags = 0;
    rdh->num_extents = 0;
    rdh->file_size = 0;
    rdh->checksum = 0;
    rdh->mode = 0666 & ~mode_mask;

    auto inode_id = Util::generate_inode_id();

    status = root.insert_inode(inode_id, new_file);
    if (status < Brufs::Status::OK) {
        fprintf(stderr, "Unable to insert the file into the root: %s\n",
            io.strstatus(status)
        );

        return 1;
    }

    status = new_file.init(inode_id, rdh);
    if (status < Brufs::Status::OK) {
        fprintf(stderr, "Unable to initialize the file: %s\n",
            io.strstatus(status)
        );

        return 1;
    }

    assert(!local_path.empty() && local_path.get_size() <= Brufs::MAX_LABEL_LENGTH);

    Brufs::DirectoryEntry entry;
    strncpy(entry.label, local_path.c_str(), Brufs::MAX_LABEL_LENGTH);
    entry.inode_id = inode_id;

    status = dir.insert(entry);
    if (status < Brufs::Status::OK) {
        fprintf(stderr, "Unable to insert the file into its parent: %s\n",
            io.strstatus(status)
        );

        status = new_file.destroy();
        if (status < Brufs::Status::OK) {
            fprintf(stderr, "Unable to destroy the new file: %s\n",
                io.strstatus(status)
            );
        }

        status = root.remove_inode(inode_id, new_file);
        if (status < Brufs::Status::OK) {
            fprintf(stderr, "Unable to remove the new file's inode: %s\n",
                io.strstatus(status)
            );
        }

        return 1;
    }

    return 0;
}
