#include "btree-common.hpp"

TEST_CASE("Bm+trees can be inserted into and removed from", "[btree]") {
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
            REQUIRE(tree.remove(i, value) == Brufs::Status::E_NOT_FOUND);
        }

        Brufs::Size count;
        REQUIRE(tree.count_values(count) == Brufs::Status::OK);
        REQUIRE(count == 0);
    }

    SECTION("can insert and then remove again") {
        const long key = 1122;
        const long init_value = 3344;
        Brufs::Status status = tree.insert(key, init_value);
        REQUIRE(status == Brufs::Status::OK);

        long ret_value;
        REQUIRE(tree.remove(key, ret_value) == Brufs::Status::OK);
        REQUIRE(ret_value == init_value);

        REQUIRE(tree.search(key, ret_value) == Brufs::Status::E_NOT_FOUND);

        Brufs::Size count;
        REQUIRE(tree.count_values(count) == Brufs::Status::OK);
        REQUIRE(count == 0);
    }

    SECTION("can insert and query again many times") {
        for (long i = 0; i < 2400; ++i) {
            CAPTURE(i);
            Brufs::Status status = tree.insert(i, i + 14616742);
            if (status != Brufs::Status::OK) printf("%s\n", Brufs::strerror(status));
            REQUIRE(status == Brufs::Status::OK);
        }

        if (0) {
            char fbuf[256];
            snprintf(fbuf, 256, "trees/tree-%lu-filled.puml", time(NULL));
            FILE *f = fopen(fbuf, "w");

            fprintf(f, "@startuml\nskinparam classBackgroundColor PeachPuff/PaleGoldenrod\n");

            char ppbuf[65536];
            tree.pretty_print_root(ppbuf, 65536);
            fprintf(f, "%s", ppbuf);

            fprintf(f, "hide empty fields\nhide empty methods\nhide << value >> stereotype\nhide << value >> circle\n@enduml\n");
            fclose(f);
        }

        for (long i = 0; i < 2400; ++i) {
            CAPTURE(i);
            long value;
            Brufs::Status status = tree.remove(i, value, true);
            if (status < 0) printf("%s\n", Brufs::strerror(status));

            // char fbuf[256];
            // snprintf(fbuf, 256, "trees/tree-%lu-%ld.puml", time(NULL), i);
            // FILE *f = fopen(fbuf, "w");

            // fprintf(f, "@startuml\nskinparam classBackgroundColor PeachPuff/PaleGoldenrod\n");

            // char ppbuf[65536];
            // tree.pretty_print_root(ppbuf, 65536);
            // fprintf(f, "%s", ppbuf);

            // fprintf(f, "hide empty fields\nhide empty methods\nhide << value >> stereotype\nhide << value >> circle\n@enduml\n");
            // fclose(f);

            REQUIRE(status == Brufs::Status::OK);
            REQUIRE(value == i + 14616742);

            status = tree.search(i, value, true);
            if (status != Brufs::Status::E_NOT_FOUND) printf("%s\n", Brufs::strerror(status));
            REQUIRE(status == Brufs::Status::E_NOT_FOUND);
        }

        Brufs::Size count;
        REQUIRE(tree.count_values(count) == Brufs::Status::OK);
        REQUIRE(count == 0);
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
            kvs.push_back(kv {rand() % 4, rand() % 4});
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

            Brufs::Status status = tree.remove(kv.k, value);
            if (status < 0) printf("%s\n", Brufs::strerror(status));
            REQUIRE(status == Brufs::Status::OK);

            results.insert({ kv.k, value });
        }

        std::set<kv> vset;
        vset.insert(kvs.begin(), kvs.end());

        REQUIRE(std::includes(vset.begin(), vset.end(), results.begin(), results.end()));

        Brufs::Size count;
        REQUIRE(tree.count_values(count) == Brufs::Status::OK);
        REQUIRE(count == 0);
    }
}
