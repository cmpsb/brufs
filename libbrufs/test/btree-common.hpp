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

#include "Brufs.hpp"

static const unsigned int DISK_SIZE = 128 * 1024 * 1024;
static const unsigned int PAGE_SIZE = 4096;

class MemAbstIO : public Brufs::AbstIO {
private:
    size_t length;
    char *buf;

public:
    MemAbstIO(size_t length) : length(length) {
        this->buf = static_cast<char *>(calloc(length, 1));
        assert(this->buf);
    }

    ~MemAbstIO() {
        free(this->buf);
    };

    Brufs::SSize read(void *buf, Brufs::Size count, Brufs::Address offset) const override {
        if (offset + count > this->length) {
            printf("rd oob 0x%lX + 0x%lX > 0x%lX\n", offset, count, this->length);
            return Brufs::Status::E_DISK_TRUNCATED;
        }

        memcpy(buf, this->buf + offset, count);

        return count;
    }

    Brufs::SSize write(const void *buf, Brufs::Size count, Brufs::Address offset) override {
        if (offset + count > this->length) {
            printf("wt oob 0x%lX + 0x%lX > 0x%lX\n", offset, count, this->length);
            return Brufs::Status::E_DISK_TRUNCATED;
        }

        memcpy(this->buf + offset, buf, count);

        return count;
    }

    const char *strstatus(Brufs::SSize eno) const override {
        (void) eno;
        return "unknown error";
    }

    Brufs::Size get_size() const override {
        return this->length;
    }
};

static std::stack<Brufs::Address> free_pages;

static Brufs::Status allocate_test_page(Brufs::Brufs &fs, Brufs::Size size, Brufs::Extent &target) {
    (void) fs;

    if (free_pages.empty()) return Brufs::Status::E_NO_SPACE;

    CHECK(size == PAGE_SIZE);

    target.offset = free_pages.top();
    target.length = size;

    free_pages.pop();

    return Brufs::Status::OK;
}

static Brufs::Status deallocate_test_page(Brufs::Brufs &fs, const Brufs::Extent &ext) {
    (void) fs;

    free_pages.push(ext.offset);

    return Brufs::Status::OK;
}
