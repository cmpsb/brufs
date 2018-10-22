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
#include "Util.hpp"

int check(int argc, char **argv) {
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
    fprintf(stderr, "%s\n", Brufs::strerror(status));
    if (status < Brufs::Status::OK) return 1;

    auto capacity = fs.get_header().num_blocks;
    Brufs::Size reserved, available, extents, in_fbt;
    status = fs.count_free_blocks(reserved, available, extents, in_fbt);
    if (status < Brufs::Status::OK) return 1;

    int available_pct = (available * 100) / capacity;
    int reserved_pct = (reserved * 100) / capacity;

    auto cap_str = Util::pretty_print_bytes(capacity);
    auto avail_str = Util::pretty_print_bytes(available);
    auto res_str = Util::pretty_print_bytes(reserved);

    printf(
        "Capacity: %s (%lu); "
        "available: %s (%lu) in %lu extents (%d%%); "
        "reserved: %s (%lu, %d%%)\n", 
        cap_str.c_str(), capacity,
        avail_str.c_str(), available, extents, available_pct, 
        res_str.c_str(), reserved, reserved_pct
    );

    int in_fbt_pct = (in_fbt * 100) / capacity;
    auto in_fbt_str = Util::pretty_print_bytes(in_fbt);

    printf("fbt at: 0x%lX, %s (%d%%)\n", 
        fs.get_header().fbt_address, in_fbt_str.c_str(), in_fbt_pct
    );

    Brufs::SSize root_count = fs.count_roots();
    printf("%lld roots\n", root_count);

    auto roots = new Brufs::RootHeader[root_count];
    status = static_cast<Brufs::Status>(fs.collect_roots(roots, root_count));
    if (status < 0) {
        fprintf(stderr, "Can't read roots: %s\n", Brufs::strerror(status));
        return 1;
    }

    for (Brufs::SSize i = 0; i < root_count; ++i) {
        Brufs::Root root(fs, roots[i]);
        printf("Root \"%s\"\n", roots[i].label);
        printf("  int at 0x%lX\n", root.get_header().int_address);
        printf("  ait at 0x%lX\n", root.get_header().ait_address);
    }

    delete[] roots;

    return status < Brufs::Status::OK;
}
