#include "btree-common.hpp"

TEST_CASE("Bm+trees support complex value types", "[btree]") {
    while (!free_pages.empty()) free_pages.pop();

    MemAbstIO io(DISK_SIZE);
    Brufs::Disk disk(&io);
    Brufs::Brufs fs(&disk);

    for (unsigned int i = 1; i < (DISK_SIZE / PAGE_SIZE); ++i) {
        free_pages.push(i * PAGE_SIZE);
    }

    Brufs::BmTree::BmTree<Brufs::Size, Brufs::Extent> tree(
        &fs, PAGE_SIZE, allocate_test_page, deallocate_test_page
    );
    REQUIRE(tree.init() == Brufs::Status::OK);

    SECTION("can insert") {
        Brufs::Extent ext { 4096, 512 };
        REQUIRE(tree.insert(ext.offset, ext) == Brufs::Status::OK);
    }

    SECTION("query on empty returns not found") {
        Brufs::Extent ext;
        REQUIRE(tree.search(4096, ext) == Brufs::Status::E_NOT_FOUND);
    }

    SECTION("can insert and query") {
        Brufs::Extent ext { 16, 8 };
        REQUIRE(tree.insert(ext.offset, ext) == Brufs::Status::OK);

        Brufs::Extent resext;
        REQUIRE(tree.search(16, resext) == Brufs::Status::OK);
        REQUIRE(resext == ext);
    }

    SECTION("can insert and query lower") {
        Brufs::Extent ext { 16, 8 };
        REQUIRE(tree.insert(ext.offset, ext) == Brufs::Status::OK);

        Brufs::Extent resext;
        REQUIRE(tree.search(8, resext) == Brufs::Status::OK);
        REQUIRE(resext == ext);
    }

    SECTION("can insert and not query higher") {
        Brufs::Extent ext { 16, 8 };
        REQUIRE(tree.insert(ext.offset, ext) == Brufs::Status::OK);

        Brufs::Extent resext;
        REQUIRE(tree.search(17, resext) == Brufs::Status::E_NOT_FOUND);
    }
}
