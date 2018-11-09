#include "catch.hpp"

#include "String.hpp"

TEST_CASE("Strings", "[standalone]") {
    SECTION("Default string is empty") {
        const Brufs::String str;

        CHECK(str.empty());
        CHECK(str.size() == 0);
        CHECK(str.c_str()[0] == 0);
    }

    SECTION("Copy of default string is also empty") {
        const Brufs::String str = Brufs::String();

        CHECK(str.empty());
        CHECK(str.size() == 0);
        CHECK(str.c_str()[0] == 0);
    }

    SECTION("Can assign empty const char*") {
        const Brufs::String str = "";

        CHECK(str.empty());
        CHECK(str.size() == 0);
        CHECK(str.c_str()[0] == 0);
    }

    SECTION("Equality works on an empty string") {
        const Brufs::String empty = "";

        CHECK(empty == Brufs::String(""));
        CHECK(Brufs::String("") == empty);
    }

    SECTION("Equality works on a single-char string") {
        const Brufs::String str = "c";

        CHECK(str == Brufs::String("c"));
        CHECK(Brufs::String("c") == str);
    }

    SECTION("Equality works on longer strings") {
        const Brufs::String str = "ooh eeh";

        CHECK(str == Brufs::String("ooh eeh"));
        CHECK(Brufs::String("ooh eeh") == str);
    }
}
