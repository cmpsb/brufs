#include "catch.hpp"

#include "PathValidator.hpp"

TEST_CASE("Path validator", "[util]") {
    Brufscli::PathValidator vtor;

    SECTION("Empty path validates with no requirements") {
        Brufs::Path path;
        CHECK_NOTHROW(vtor.validate(path, false, false));
    }

    SECTION("Empty path does not validate with partition required") {
        Brufs::Path path;
        CHECK_THROWS_AS(vtor.validate(path, true, false), Brufscli::NoPartitionException);
    }

    SECTION("Empty path does not validate with root required") {
        Brufs::Path path;
        CHECK_THROWS_AS(vtor.validate(path, false, true), Brufscli::NoRootException);
    }

    SECTION("Empty path does not validate with both required") {
        Brufs::Path path;
        CHECK_THROWS_AS(vtor.validate(path, true, true), Brufscli::NoPartitionException);
    }

    SECTION("Full path does validate with both required") {
        Brufs::Path path("part", "root", Brufs::Vector<Brufs::String>::of("a", "b"));
        CHECK_NOTHROW(vtor.validate(path, true, true));
    }
}
