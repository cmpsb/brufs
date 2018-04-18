#include <algorithm>
#include <stack>
#include <vector>
#include <random>

#include <cstdlib>
#include <cassert>
#include <cstring>
#include <cstdio>
#include <ctime>

#include "catch.hpp"

#include "btree.hpp"

static const unsigned int DISK_SIZE = 128 * 1024 * 1024;
static const unsigned int PAGE_SIZE = 4096;

class mem_abstio : public brufs::abstio {
private:
    size_t length;
    char *buf;

public:
    mem_abstio(size_t length) : length(length) {
        this->buf = static_cast<char *>(malloc(length));
        assert(this->buf);
    }

    ~mem_abstio() {};

    brufs::ssize read(void *buf, brufs::size count, brufs::address offset) const override {
        if (offset + count > this->length) {
            printf("rd oob 0x%lX + 0x%lX > 0x%lX\n", offset, count, this->length);
            return brufs::status::E_DISK_TRUNCATED;
        }

        memcpy(buf, this->buf + offset, count);

        return count;
    }

    brufs::ssize write(const void *buf, brufs::size count, brufs::address offset) override {
        if (offset + count > this->length) {
            printf("wt oob 0x%lX + 0x%lX > 0x%lX\n", offset, count, this->length);
            return brufs::status::E_DISK_TRUNCATED;
        }

        memcpy(this->buf + offset, buf, count);

        return count;
    }

    const char *strstatus(brufs::ssize eno) const override {
        (void) eno;
        return "unknown error";
    }

    brufs::size get_size() const override {
        return this->length;
    }
};

static std::stack<brufs::address> free_pages;

static brufs::status allocate_test_page(brufs::brufs &fs, brufs::size size, brufs::extent &target) {
    (void) fs;

    if (free_pages.empty()) return brufs::status::E_NO_SPACE;

    assert(size == PAGE_SIZE);

    target.offset = free_pages.top();
    target.length = size;

    free_pages.pop();

    return brufs::status::OK;
}

TEST_CASE("Bm+trees can be inserted into and queried", "[btree]") {
    while (!free_pages.empty()) free_pages.pop();

    mem_abstio io(DISK_SIZE);
    brufs::disk disk(&io);
    brufs::brufs fs(&disk);

    for (unsigned int i = 1; i < (DISK_SIZE / PAGE_SIZE); ++i) {
        free_pages.push(i * PAGE_SIZE);
    }

    brufs::bmtree::bmtree<long, long, allocate_test_page> tree(fs, PAGE_SIZE);
    REQUIRE(tree.init() == brufs::status::OK);

    SECTION("an empty tree has no values") {
        brufs::size count;
        REQUIRE(tree.count_values(count) == brufs::status::OK);
        REQUIRE(count == 0);
    }

    SECTION("queries on an empty tree always error") {
        for (long i = -10; i <= 10; ++i) {
            long value;
            REQUIRE(tree.search(i, value) == brufs::status::E_NOT_FOUND);
        }

        brufs::size count;
        REQUIRE(tree.count_values(count) == brufs::status::OK);
        REQUIRE(count == 0);
    }

    SECTION("can insert") {
        REQUIRE(tree.insert(0, 0) == brufs::status::OK);

        brufs::size count;
        REQUIRE(tree.count_values(count) == brufs::status::OK);
        REQUIRE(count == 1);
    }

    SECTION("can insert and then query again") {
        const long key = 1122;
        const long init_value = 3344;
        brufs::status status = tree.insert(key, init_value);
        REQUIRE(status == brufs::status::OK);

        long ret_value;
        REQUIRE(tree.search(key, ret_value) == brufs::status::OK);
        REQUIRE(ret_value == init_value);

        brufs::size count;
        REQUIRE(tree.count_values(count) == brufs::status::OK);
        REQUIRE(count == 1);
    }

    // SECTION("can insert and query again many times") {
    //     for (long i = 1000000; i >= -1000000; --i) {
    //         CAPTURE(i);
    //         brufs::status status = tree.insert(i, i * i);
    //         if (status != brufs::status::OK) printf("%s\n", brufs::strerror(status));
    //         REQUIRE(status == brufs::status::OK);
    //     }

    //     brufs::size count;
    //     REQUIRE(tree.count_values(count) == brufs::status::OK);
    //     REQUIRE(count == 2000001);

    //     for (long i = -1000000; i <= 1000000; ++i) {
    //         CAPTURE(i);
    //         long value;
    //         REQUIRE(tree.search(i, value) == brufs::status::OK);
    //         REQUIRE(value == i * i);
    //     }
    // }

    SECTION("can insert and query in a random order") {
        srand(6);

        std::default_random_engine reng(6);

        struct kv {
            long k;
            long v;
        };

        std::vector<kv> kvs;

        for (long i = 0; i < 24; ++i) {
            kvs.push_back(kv {rand() % 100, rand() % 100});
        }

        std::shuffle(kvs.begin(), kvs.end(), reng);

        unsigned int idx = 0;
        unsigned long gen = time(NULL);

        for (const auto kv : kvs) {
            char fbuf[256];
            snprintf(fbuf, 256, "trees/tree-%lu-%u.puml", gen, idx);
            FILE *f = fopen(fbuf, "w");

            fprintf(f, "@startuml\n");

            char ppbuf[65536];
            tree.pretty_print_root(ppbuf, 65536);
            fprintf(f, "%s", ppbuf);

            fprintf(f, "hide empty fields\nhide empty methods\n@enduml\n");
            fclose(f);

            printf("nnodes: %u\n", tree.root);

            brufs::status status = tree.insert(kv.k, kv.v);
            REQUIRE(status == brufs::status::OK);

            ++idx;
        }

        std::shuffle(kvs.begin(), kvs.end(), reng);

        for (const auto kv : kvs) {
            CAPTURE(kv.k);
            long value;
            REQUIRE(tree.search(kv.k, value) == brufs::status::OK);
            REQUIRE(value == kv.v);
        }

        brufs::size count;
        REQUIRE(tree.count_values(count) == brufs::status::OK);
        REQUIRE(count == 24);
    }
}
