#include <stack>

#include <cstdlib>
#include <cassert>
#include <cstring>

#include "catch.hpp"

#include "btree.hpp"

static const unsigned int DISK_SIZE = 8 * 1024 * 1024;
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

TEST_CASE("Bm+trees support complex value types", "[btree]") {
    while (!free_pages.empty()) free_pages.pop();

    mem_abstio io(DISK_SIZE);
    brufs::disk disk(&io);
    brufs::brufs fs(&disk);

    for (unsigned int i = 1; i < (DISK_SIZE / PAGE_SIZE); ++i) {
        free_pages.push(i * PAGE_SIZE);
    }

    brufs::bmtree::bmtree<brufs::size, brufs::extent, allocate_test_page> tree(fs, PAGE_SIZE);
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
