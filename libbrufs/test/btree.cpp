#include "btree-common.hpp"

TEST_CASE("Bm+trees can be inserted into and queried", "[btree]") {
    while (!free_pages.empty()) free_pages.pop();

    MemAbstIO io(DISK_SIZE);
    Brufs::Disk disk(&io);
    Brufs::Brufs fs(&disk);

    for (unsigned int i = 1; i < (DISK_SIZE / PAGE_SIZE); ++i) {
        free_pages.push(i * PAGE_SIZE);
    }

    Brufs::BmTree::BmTree<long, long> tree(
        &fs, PAGE_SIZE, allocate_test_page, deallocate_test_page
    );
    REQUIRE(tree.init() == Brufs::Status::OK);

    SECTION("an empty tree has no values") {
        Brufs::Size count;
        REQUIRE(tree.count_values(count) == Brufs::Status::OK);
        REQUIRE(count == 0);
    }

    SECTION("queries on an empty tree always error") {
        for (long i = -10; i <= 10; ++i) {
            long value;
            REQUIRE(tree.search(i, value) == Brufs::Status::E_NOT_FOUND);
        }

        Brufs::Size count;
        REQUIRE(tree.count_values(count) == Brufs::Status::OK);
        REQUIRE(count == 0);
    }

    SECTION("can insert") {
        REQUIRE(tree.insert(0, 0L) == Brufs::Status::OK);

        Brufs::Size count;
        REQUIRE(tree.count_values(count) == Brufs::Status::OK);
        REQUIRE(count == 1);
    }

    SECTION("can insert and then query again") {
        const long key = 1122;
        const long init_value = 3344;
        Brufs::Status status = tree.insert(key, init_value);
        REQUIRE(status == Brufs::Status::OK);

        long ret_value;
        REQUIRE(tree.search(key, ret_value) == Brufs::Status::OK);
        REQUIRE(ret_value == init_value);

        Brufs::Size count;
        REQUIRE(tree.count_values(count) == Brufs::Status::OK);
        REQUIRE(count == 1);
    }

    SECTION("can insert and query in a random order") {
        srand(6);

        std::default_random_engine reng(6);

        struct kv {
            long k;
            long v;

            auto operator==(const kv &other) const {
                return this->k == other.k && this->v == other.v;
            }

            auto operator<(const kv &other) const {
                return this->k < other.k || (this->k == other.k && this->v < other.v);
            }
        };

        std::vector<kv> kvs;

        for (long i = 0; i < 24; ++i) {
            kvs.push_back(kv {rand() % 100, rand() % 100});
        }

        std::shuffle(kvs.begin(), kvs.end(), reng);

        for (const auto kv : kvs) {
            Brufs::Status status = tree.insert(kv.k, kv.v);
            REQUIRE(status == Brufs::Status::OK);
        }

        std::shuffle(kvs.begin(), kvs.end(), reng);

        std::set<kv> results;

        for (const auto kv : kvs) {
            CAPTURE(kv.k);
            long value;
            REQUIRE(tree.search(kv.k, value) == Brufs::Status::OK);
            results.insert({ kv.k, value });
        }

        std::set<kv> vset;
        vset.insert(kvs.begin(), kvs.end());

        REQUIRE(std::includes(vset.begin(), vset.end(), results.begin(), results.end()));

        Brufs::Size count;
        REQUIRE(tree.count_values(count) == Brufs::Status::OK);
        REQUIRE(count == 24);
    }

    SECTION("Can count used space") {
        Brufs::Size size;
        REQUIRE(tree.count_used_space(size) == Brufs::Status::OK);

        CHECK(size == PAGE_SIZE);
    }

    SECTION("can query the first value") {
        REQUIRE(tree.insert(15, 99) == Brufs::Status::OK);
        REQUIRE(tree.insert(25, 9) == Brufs::Status::OK);
        REQUIRE(tree.insert(2, 999) == Brufs::Status::OK);

        long first;
        REQUIRE(tree.get_first(first) == Brufs::Status::OK);
        REQUIRE(first == 999);
    }

    SECTION("can query the last value") {
        REQUIRE(tree.insert(15, 9) == Brufs::Status::OK);
        REQUIRE(tree.insert(25, 99) == Brufs::Status::OK);
        REQUIRE(tree.insert(2, 999) == Brufs::Status::OK);

        long last;
        REQUIRE(tree.get_last(last) == Brufs::Status::OK);
        REQUIRE(last == 99);
    }

    SECTION("can destroy the tree") {
        REQUIRE(tree.destroy() == Brufs::Status::OK);

        // The first page is reserved, like nullptr is
        REQUIRE(free_pages.size() + 1 == DISK_SIZE / PAGE_SIZE);
    }

    SECTION("can walk the tree") {
        unsigned long total = 0;
        for (int i = 0; i < 1000; total += (i % 9), ++i) {
            REQUIRE(tree.insert(i, i % 9) == Brufs::Status::OK);
        }

        unsigned long walked_total = 0;
        REQUIRE(tree.walk<unsigned long *>([](long &v, unsigned long *p) {
            *p += v;
            return Brufs::Status::OK;
        }, &walked_total) == Brufs::Status::OK);

        CHECK(total == walked_total);
    }
}
