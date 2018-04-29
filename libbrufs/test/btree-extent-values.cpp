#include "btree-common.hpp"

TEST_CASE("Bm+trees support complex value types", "[btree]") {
    while (!free_pages.empty()) free_pages.pop();

    mem_abstio io(DISK_SIZE);
    brufs::disk disk(&io);
    brufs::brufs fs(&disk);

    for (unsigned int i = 1; i < (DISK_SIZE / PAGE_SIZE); ++i) {
        free_pages.push(i * PAGE_SIZE);
    }

    brufs::bmtree::bmtree<brufs::size, brufs::extent> tree(
        &fs, PAGE_SIZE, allocate_test_page, deallocate_test_page
    );
    REQUIRE(tree.init() == brufs::status::OK);

    SECTION("can insert") {
        brufs::extent ext { 4096, 512 };
        REQUIRE(tree.insert(ext.offset, ext) == brufs::status::OK);
    }

    SECTION("query on empty returns not found") {
        brufs::extent ext;
        REQUIRE(tree.search(4096, ext) == brufs::status::E_NOT_FOUND);
    }

    SECTION("can insert and query") {
        brufs::extent ext { 16, 8 };
        REQUIRE(tree.insert(ext.offset, ext) == brufs::status::OK);

        brufs::extent resext;
        REQUIRE(tree.search(16, resext) == brufs::status::OK);
        REQUIRE(resext == ext);
    }

    SECTION("can insert and query lower") {
        brufs::extent ext { 16, 8 };
        REQUIRE(tree.insert(ext.offset, ext) == brufs::status::OK);

        brufs::extent resext;
        REQUIRE(tree.search(8, resext) == brufs::status::OK);
        REQUIRE(resext == ext);
    }

    SECTION("can insert and not query higher") {
        brufs::extent ext { 16, 8 };
        REQUIRE(tree.insert(ext.offset, ext) == brufs::status::OK);

        brufs::extent resext;
        REQUIRE(tree.search(17, resext) == brufs::status::E_NOT_FOUND);
    }
}
