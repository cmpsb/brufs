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
#include <cstring>
#include <cerrno>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

#include "libbrufs.hpp"

#include "FdAbst.hpp"
#include "prompt.hpp"

int add_root(int argc, char **argv) {
    if (argc < 2) {
        fprintf(stderr, "Specify a file.\n");
        return 2;
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

    std::string label = prompt_string("Label", "", Brufs::MAX_LABEL_LENGTH - 1);

    Brufs::RootHeader root_header;
    strncpy(root_header.label, label.c_str(), Brufs::MAX_LABEL_LENGTH);
    root_header.inode_size = 128;
    root_header.inode_header_size = sizeof(Brufs::InodeHeader);
    root_header.max_extent_length = 16 * fs.get_header().cluster_size;

    Brufs::Root root(fs, root_header);
    status = fs.add_root(root);
    if (status < Brufs::Status::OK) {
        fprintf(stderr, "Unable to insert the root into the filesystem: %s\n",
            Brufs::strerror(status)
        );

        return 1;
    }

    status = root.init();
    if (status < Brufs::Status::OK) {
        fprintf(stderr, "Unable to initialize the root: %s\n",
            Brufs::strerror(status)
        );

        return 1;
    }

    mode_t mode_mask = umask(0);
    umask(mode_mask);

    Brufs::Directory root_dir(root);

    auto rdh = root_dir.get_header();
    rdh->created = Brufs::Timestamp::now();
    rdh->last_modified = Brufs::Timestamp::now();
    rdh->owner = geteuid();
    rdh->group = getegid();
    rdh->num_links = 1;
    rdh->type = Brufs::InodeType::DIRECTORY;
    rdh->flags = 0;
    rdh->num_extents = 0;
    rdh->file_size = 0;
    rdh->checksum = 0;
    rdh->mode = 0777 & ~mode_mask;

    status = root.insert_inode(Brufs::ROOT_DIR_INODE_ID, root_dir.get_header());
    if (status < Brufs::Status::OK) {
        fprintf(stderr, "Unable to insert the root directory into the root: %s\n",
            Brufs::strerror(status)
        );

        return 1;
    }

    status = root_dir.init(Brufs::ROOT_DIR_INODE_ID, rdh);
    if (status < Brufs::Status::OK) {
        fprintf(stderr, "Unable to initialize the root directory: %s\n",
            Brufs::strerror(status)
        );

        return 1;
    }

    Brufs::Directory root_dir_check(root);
    status = root.open_directory(Brufs::ROOT_DIR_INODE_ID, root_dir_check);
    root_dir_check.count();

    printf("%s\n", Brufs::strerror(status));

    return status != Brufs::Status::OK;
}
