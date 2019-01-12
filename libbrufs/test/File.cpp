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
#include "File.hpp"
#include "EntityCreator.hpp"

static constexpr size_t NORMAL_DISK_SIZE = 32 * 1024 * 1024;
static constexpr Brufs::InodeId INODE_ID = 65536;

class StaticInodeIdGenerator : public Brufs::InodeIdGenerator {
public:
    Brufs::InodeId generate() const override {
        return INODE_ID;
    }
};

TEST_CASE("Can read and write files", "[File]") {
    MemIO mem_io(NORMAL_DISK_SIZE);
    Brufs::Disk disk(&mem_io);
    Brufs::Brufs fs(&disk);

    Brufs::Header proto;
    proto.cluster_size_exp = 12;
    proto.sc_low_mark = 12;
    proto.sc_high_mark = 24;

    REQUIRE(fs.init(proto) == Brufs::Status::OK);

    Brufs::RootHeader root_header;
    root_header.set_label("root-name");

    Brufs::Root root(fs, root_header);
    REQUIRE(root.init() == Brufs::Status::OK);
    REQUIRE(fs.add_root(root) == Brufs::Status::OK);

    Brufs::Inode inode(root);
    StaticInodeIdGenerator inode_id_generator;
    Brufs::EntityCreator entity_creator(inode_id_generator);

    Brufs::Path path("root-name", Brufs::Vector<Brufs::String>::of("thing"));
    Brufs::File file(root);
    Brufs::InodeHeaderBuilder ihb;
    REQUIRE(entity_creator.create_file(path, ihb, file) == Brufs::Status::OK);

    SECTION("Can't read into an invalid buffer") {
        CHECK(file.read(nullptr, 0, 0) == Brufs::Status::E_INVALID_ARGUMENT);
    }

    SECTION("Can read from an empty file") {
        char buf[1];
        CHECK(file.read(buf, 0, 0) == 0);
    }
}
