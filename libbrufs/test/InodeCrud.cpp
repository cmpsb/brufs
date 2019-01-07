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

#include "catch.hpp"

#include "MemIO.hpp"
#include "Brufs.hpp"
#include "Root.hpp"
#include "InodeHeader.hpp"
#include "Inode.hpp"

static constexpr size_t NORMAL_DISK_SIZE = 32 * 1024 * 1024;
static constexpr Brufs::InodeId INODE_ID = 65536;

TEST_CASE("Can add, query, update, and remove inodes", "[Inode]") {
    MemIO mem_io(NORMAL_DISK_SIZE);
    Brufs::Disk disk(&mem_io);
    Brufs::Brufs fs(&disk);

    Brufs::Header proto;
    proto.cluster_size_exp = 12;
    proto.sc_low_mark = 12;
    proto.sc_high_mark = 24;

    REQUIRE(fs.init(proto) == Brufs::Status::OK);

    Brufs::RootHeader root_header;
    strncpy(root_header.label, "root-name", Brufs::MAX_LABEL_LENGTH);
    root_header.inode_size = 128;
    root_header.inode_header_size = sizeof(Brufs::InodeHeader);
    root_header.max_extent_length = 8 * fs.get_header().cluster_size;

    Brufs::Root root(fs, root_header);
    REQUIRE(root.init() == Brufs::Status::OK);
    REQUIRE(fs.add_root(root) == Brufs::Status::OK);

    auto inode_header = root.create_inode_header();
    REQUIRE(inode_header != nullptr);

    inode_header->created = inode_header->last_modified = Brufs::Timestamp::now();
    inode_header->owner = inode_header->group = 1000;
    inode_header->num_links = 2;
    inode_header->type = Brufs::InodeType::FILE;
    inode_header->flags = 0;
    inode_header->mode = 0644;
    inode_header->file_size = 0xF00F;
    inode_header->checksum = 0;

    Brufs::Inode inode(root);
    REQUIRE(inode.init(INODE_ID, inode_header) == Brufs::Status::OK);

    SECTION("Can add an inode") {
        REQUIRE(root.insert_inode(INODE_ID, inode) == Brufs::Status::OK);
    }

    SECTION("Can't add an inode that already exists") {
        REQUIRE(root.insert_inode(INODE_ID, inode) == Brufs::Status::OK);
        REQUIRE(root.insert_inode(INODE_ID, inode) == Brufs::Status::E_EXISTS);
    }

    SECTION("Can add an inode and look it up again") {
        Brufs::Inode found_inode(root);
        REQUIRE(root.insert_inode(INODE_ID, inode) == Brufs::Status::OK);
        REQUIRE(root.find_inode(INODE_ID, found_inode) == Brufs::Status::OK);
        REQUIRE(
            memcmp(inode.get_header(), found_inode.get_header(), sizeof(Brufs::InodeHeader)) == 0
        );
    }

    SECTION("Can update an inode") {
        REQUIRE(root.insert_inode(INODE_ID, inode) == Brufs::Status::OK);

        inode.get_header()->group = 1111;
        REQUIRE(root.update_inode(INODE_ID, inode) == Brufs::Status::OK);

        Brufs::Inode found_inode(root);
        REQUIRE(root.find_inode(INODE_ID, found_inode) == Brufs::Status::OK);
        REQUIRE(
            memcmp(inode.get_header(), found_inode.get_header(), sizeof(Brufs::InodeHeader)) == 0
        );
    }

    root.destroy_inode_header(inode_header);
}
