#include "catch.hpp"

#include "PathParser.hpp"

TEST_CASE("Path operations", "[util]") {
    SECTION("The parent of the root is the root") {
        const Brufs::Path path;

        CHECK(path.get_parent() == path);
    }

    SECTION("The parent of a path is the path minus the deepest component") {
        const Brufs::Path path(Brufs::Vector<Brufs::String>::of("stuff", "thing"));
        const auto parent = path.get_parent();

        CHECK(parent.get_components().get_size() == 1);
        CHECK(parent.get_components()[0] == "stuff");
    }
}
