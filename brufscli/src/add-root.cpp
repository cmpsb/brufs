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

#include "brufs.hpp"

#include "fd_abst.hpp"
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

    fd_abst io(iofd);
    brufs::disk disk(&io);
    brufs::brufs fs(&disk);

    brufs::status status = fs.get_status();
    if (status < brufs::status::OK) {
        fprintf(stderr, "%s\n", brufs::strerror(status));
        return 1;
    }

    std::string label = prompt_string("Label", "", brufs::MAX_LABEL_LENGTH - 1);
    brufs::root_header root;
    strncpy(root.label, label.c_str(), brufs::MAX_LABEL_LENGTH);

    status = fs.add_root(root);

    printf("%s\n", brufs::strerror(status));

    return status != brufs::status::OK;
}
