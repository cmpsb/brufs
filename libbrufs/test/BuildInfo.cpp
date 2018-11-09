#include "catch.hpp"

#include "BuildInfo.hpp"


TEST_CASE("Build information is sound", "[build]") {
    const auto info = Brufs::BuildInfo::get();

    SECTION("There are flags") {
        CHECK(!info.flags.empty());
    }

    SECTION("There is a build date (not necessarily correct)") {
        CHECK(!info.build_date.empty());
    }

    SECTION("The build type is either debug or release") {
        CHECK((info.is_debug() ^ info.is_release()));
    }

    SECTION("Other flag checks don't crash (any value is valid)") {
        info.is_from_git();
        info.is_dirty();
    }
}
