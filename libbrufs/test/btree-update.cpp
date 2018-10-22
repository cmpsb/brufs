#include "btree-common.hpp"

struct thing {
    unsigned int k;
    unsigned int v;
};

namespace Brufs { namespace BmTree {

template <>
bool equiv_values(const thing *current, const thing *replacement) {
    return current->k == replacement->k;
}

}}

TEST_CASE("Hm+trees filter updates correctly", "[htree]") {
    while (!free_pages.empty()) free_pages.pop();

    MemAbstIO io(DISK_SIZE);
    Brufs::Disk disk(&io);
    Brufs::Brufs fs(&disk);

    for (unsigned int i = 1; i < (DISK_SIZE / PAGE_SIZE); ++i) {
        free_pages.push(i * PAGE_SIZE);
    }

    Brufs::BmTree::BmTree<unsigned int, thing> tree(
        &fs, PAGE_SIZE, allocate_test_page, deallocate_test_page
    );
    REQUIRE(tree.init() == Brufs::Status::OK);

    SECTION("can't update empty tree") {
        REQUIRE(tree.update(22, {22, 0}) == Brufs::Status::E_NOT_FOUND);
    }

    SECTION("can update single element") {
        REQUIRE(tree.insert(22, {22, 0}) == Brufs::Status::OK);
        REQUIRE(tree.update(22, {22, 1}) == Brufs::Status::OK);

        thing thing;
        REQUIRE(tree.search(22, thing) == Brufs::Status::OK);
        REQUIRE(thing.k == 22);
        REQUIRE(thing.v == 1);
    }

    SECTION("can update single element in multi-element tree") {
        REQUIRE(tree.insert(19, {19, 0}) == Brufs::Status::OK);
        REQUIRE(tree.insert(22, {22, 0}) == Brufs::Status::OK);
        REQUIRE(tree.insert(44, {44, 0}) == Brufs::Status::OK);

        REQUIRE(tree.update(22, {22, 1}) == Brufs::Status::OK);

        thing thing;
        REQUIRE(tree.search(19, thing) == Brufs::Status::OK);
        REQUIRE(thing.k == 19);
        REQUIRE(thing.v == 0);

        REQUIRE(tree.search(22, thing) == Brufs::Status::OK);
        REQUIRE(thing.k == 22);
        REQUIRE(thing.v == 1);

        REQUIRE(tree.search(44, thing) == Brufs::Status::OK);
        REQUIRE(thing.k == 44);
        REQUIRE(thing.v == 0);
    }

    SECTION("can update multiple elements") {
        REQUIRE(tree.insert(22, {22, 0}) == Brufs::Status::OK);
        REQUIRE(tree.insert(22, {22, 0}) == Brufs::Status::OK);
        REQUIRE(tree.insert(22, {22, 0}) == Brufs::Status::OK);

        REQUIRE(tree.update(22, {22, 1}) == Brufs::Status::OK);

        thing things[3];
        REQUIRE(tree.search(22, things, 3) == 3);

        REQUIRE(things[0].k == 22);
        REQUIRE(things[0].v == 1);

        REQUIRE(things[1].k == 22);
        REQUIRE(things[1].v == 1);

        REQUIRE(things[2].k == 22);
        REQUIRE(things[2].v == 1);
    }

    SECTION("can update many times") {
        for (unsigned int i = 0; i < 1000; ++i) {
            REQUIRE(tree.insert(i / 4, {i, i / 4}) == Brufs::Status::OK);
        }

        for (unsigned int i = 0; i < 1000 / 4; ++i) {
            REQUIRE(tree.update(i, {i * 4, 0}) == Brufs::Status::OK);
        }

        for (unsigned int i = 0; i < 1000 / 4; ++i) {
            CAPTURE(i);

            thing things[4];
            REQUIRE(tree.search(i, things, 4, true) == 4);

            for (unsigned int k = 0; k < 4; ++k) {
                if (things[k].k % 4 == 0) {
                    REQUIRE(things[k].v == 0);
                } else {
                    REQUIRE(things[k].v == i);
                }
            }
        }
    }
}
