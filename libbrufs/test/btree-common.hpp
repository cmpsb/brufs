#include <algorithm>
#include <stack>
#include <vector>
#include <random>
#include <set>
#include <algorithm>

#include <cstdlib>
#include <cassert>
#include <cstring>
#include <cstdio>
#include <ctime>
#include <cmath>

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

static void deallocate_test_page(brufs::brufs &fs, const brufs::extent &ext) {
    (void) fs;

    free_pages.push(ext.offset);
}
