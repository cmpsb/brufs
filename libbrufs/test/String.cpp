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

    SECTION("Can assign arbitrary const char*") {
        const char *very_arbitrary = "very arbitrary!";

        const Brufs::String str = very_arbitrary;

        CHECK(!str.empty());
        CHECK(str.size() == strlen(very_arbitrary));
        CHECK(memcmp(very_arbitrary, str.c_str(), strlen(very_arbitrary)) == 0);
        CHECK(str.front() == 'v');
        CHECK(str.back() == '!');

        CHECK(str[4] == ' ');
        CHECK(str[7] == 'b');
    }

    SECTION("Can use non-default constructor") {
        const char *bare_string = "so dangerous D:<";

        const auto str = Brufs::String(bare_string);

        CHECK(!str.empty());
        CHECK(str.size() == strlen(bare_string));
        CHECK(memcmp(bare_string, str.c_str(), strlen(bare_string)) == 0);
        CHECK(str.front() == 's');
        CHECK(str.back() == '<');

        CHECK(str[1] == 'o');
        CHECK(str[6] == 'g');
        CHECK(str[15] == '<');
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

    SECTION("Inequality works on an empty string") {
        const Brufs::String empty = "";

        CHECK(empty != Brufs::String("stuff"));
        CHECK(Brufs::String("other stuff") != empty);
    }

    SECTION("Inequality works on a single-char string") {
        const Brufs::String str = "c";

        CHECK(str != Brufs::String("e"));
        CHECK(Brufs::String("d") != str);
    }

    SECTION("Inequality works on longer strings") {
        const Brufs::String str = "ooh eeh";

        CHECK(str != Brufs::String("ooh ah aah"));
        CHECK(Brufs::String("ooh ah aah") != str);
    }

    SECTION("Can concatenate two arbitrary strings") {
        const Brufs::String etaoin = "thing";
        const Brufs::String shrdlu = "tester";

        const Brufs::String concat = etaoin + shrdlu;

        CHECK(concat.size() == etaoin.size() + shrdlu.size());
        CHECK(concat.front() == etaoin.front());
        CHECK(concat.back() == shrdlu.back());

        CHECK(etaoin + shrdlu == Brufs::String("thingtester"));
    }

    SECTION("Can split by a character") {
        const Brufs::String directions_str = "north;east;south;west";
        const auto directions = directions_str.split(';');

        CHECK(directions.get_size() == 4);
        CHECK(directions[0] == "north");
        CHECK(directions[1] == "east");
        CHECK(directions[2] == "south");
        CHECK(directions[3] == "west");
    }
}
