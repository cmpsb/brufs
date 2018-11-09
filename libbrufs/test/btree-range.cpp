#include "btree-common.hpp"

TEST_CASE("Bm+trees can be queried by range or collisions", "[btree][htree]") {
    while (!free_pages.empty()) free_pages.pop();

    MemAbstIO io(DISK_SIZE);
    Brufs::Disk disk(&io);
    Brufs::Brufs fs(&disk);

    for (unsigned int i = 1; i < (DISK_SIZE / PAGE_SIZE); ++i) {
        free_pages.push(i * PAGE_SIZE);
    }

    Brufs::BmTree::BmTree<int, int> tree(&fs, PAGE_SIZE, allocate_test_page, deallocate_test_page);
    REQUIRE(tree.init() == Brufs::Status::OK);

    SECTION("can query ranges") {
        const int max_values = 10000;

        for (long i = 0; i < max_values; ++i) {
            REQUIRE(tree.insert(i, i + 10) == Brufs::Status::OK);
        }

        const int expected_num = max_values / 2;

        int results[expected_num];
        int num_results = tree.search(expected_num + 2111, results, expected_num);

        REQUIRE(num_results == expected_num);

        for (int i = 0; i < num_results; ++i) {
            REQUIRE(results[i] == 7121 - i);
        }
    }

    SECTION("can query collisions (strict)") {
        const int max_values = 10000;

        for (long i = 0; i < max_values; ++i) {
            REQUIRE(tree.insert(i / 10, i) == Brufs::Status::OK);
        }

        const int expected_num = 10;

        int results[expected_num];
        int num_results = tree.search(9, results, expected_num, true);

        REQUIRE(num_results == expected_num);

        std::set<int> eset;
        std::set<int> rset;
        for (int i = 0; i < expected_num; ++i) {
            eset.insert(90 + i);
            rset.insert(results[i]);
        }

        REQUIRE(std::includes(eset.begin(), eset.end(), rset.begin(), rset.end()));
    }
}
