#include "catch.hpp"

#include "PathParser.hpp"

TEST_CASE("Parsing paths", "[util]") {
    Brufs::PathParser parser;

    SECTION("Can parse bare slash") {
        const auto path = parser.parse("/");

        CHECK_FALSE(path.has_partition());
        CHECK(path.get_partition().empty());
        CHECK_FALSE(path.has_root());
        CHECK(path.get_root().empty());
        CHECK(path.get_components().empty());
    }

    SECTION("Can parse longer path without partition and root") {
        const auto path = parser.parse("/three/dirs/deep/");

        CHECK_FALSE(path.has_partition());
        CHECK_FALSE(path.has_root());

        const auto components = path.get_components();

        CHECK(components.get_size() == 3);
        CHECK(components[0] == "three");
        CHECK(components[1] == "dirs");
        CHECK(components[2] == "deep");
    }

    SECTION("Can parse longer path with root, without partition") {
        const auto path = parser.parse("www:/sites/example.com/htdocs");

        CHECK_FALSE(path.has_partition());

        CHECK(path.has_root());
        CHECK(path.get_root() == "www");

        const auto components = path.get_components();

        CHECK(components.get_size() == 3);
        CHECK(components[0] == "sites");
        CHECK(components[1] == "example.com");
        CHECK(components[2] == "htdocs");
    }

    SECTION("Can parse longer path with all bells and whistles") {
        const auto path = parser.parse("disk.img:system:/data/music.7z");

        CHECK(path.has_partition());
        CHECK(path.get_partition() == "disk.img");

        CHECK(path.has_root());
        CHECK(path.get_root() == "system");

        const auto components = path.get_components();

        CHECK(components.get_size() == 2);
        CHECK(components[0] == "data");
        CHECK(components[1] == "music.7z");
    }

    SECTION("Can parse with missing root") {
        const auto path = parser.parse("disk.img::/what/is/this/I/don't/even");

        CHECK(path.has_partition());
        CHECK(path.get_partition() == "disk.img");

        CHECK_FALSE(path.has_root());

        const auto components = path.get_components();

        CHECK(components.get_size() == 6);
        CHECK(components[0] == "what");
        CHECK(components[1] == "is");
        CHECK(components[2] == "this");
        CHECK(components[3] == "I");
        CHECK(components[4] == "don't");
        CHECK(components[5] == "even");
    }
}

TEST_CASE("Stringifying paths", "[util]") {
    Brufs::PathParser parser;

    SECTION("Can write simplest path") {
        const Brufs::Path path;
        const auto str = parser.unparse(path);

        CHECK(str == "/");
    }

    SECTION("Can write simple, multi-component path") {
        const Brufs::Path path{Brufs::Vector<Brufs::String>::of("hello", "there")};
        const auto str = parser.unparse(path);

        CHECK(str == "/hello/there");
    }

    SECTION("Can write path with root") {
        const Brufs::Path path("data", Brufs::Vector<Brufs::String>::of("dust", "test.py"));
        const auto str = parser.unparse(path);

        CHECK(str == "data:/dust/test.py");
    }

    SECTION("Can write path with partition and root") {
        const Brufs::Path path("vdisk0", "sys", Brufs::Vector<Brufs::String>::of("config", "boot"));
        const auto str = parser.unparse(path);

        CHECK(str == "vdisk0:sys:/config/boot");
    }

    SECTION("Can write weird path with partition but no root") {
        const Brufs::Path path("mem", "", Brufs::Vector<Brufs::String>::of("a", "b", "abcd"));
        const auto str = parser.unparse(path);

        CHECK(str == "/a/b/abcd");
    }
}
