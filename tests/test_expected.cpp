#include <catch2/catch_test_macros.hpp>
#include <coroute/util/expected.hpp>
#include <string>

TEST_CASE("expected<T, E> basic operations", "[expected]") {
    using coroute::expected;
    
    SECTION("default construction") {
        expected<int, std::string> e;
        REQUIRE(e.has_value());
    }
    
    SECTION("value construction") {
        expected<int, std::string> e = 42;
        REQUIRE(e.has_value());
        REQUIRE(*e == 42);
        REQUIRE(e.value() == 42);
    }
    
    SECTION("error construction") {
        expected<int, std::string> e = coroute::unexpected<std::string>("error");
        REQUIRE(!e.has_value());
        REQUIRE(e.error() == "error");
    }
    
    SECTION("value_or") {
        expected<int, std::string> good = 42;
        expected<int, std::string> bad = coroute::unexpected<std::string>("error");
        
        REQUIRE(good.value_or(0) == 42);
        REQUIRE(bad.value_or(0) == 0);
    }
    
    SECTION("boolean conversion") {
        expected<int, std::string> good = 42;
        expected<int, std::string> bad = coroute::unexpected<std::string>("error");
        
        REQUIRE(static_cast<bool>(good) == true);
        REQUIRE(static_cast<bool>(bad) == false);
    }
}

TEST_CASE("expected<void, E> operations", "[expected]") {
    using coroute::expected;
    
    SECTION("default construction") {
        expected<void, std::string> e;
        REQUIRE(e.has_value());
    }
    
    SECTION("error construction") {
        expected<void, std::string> e = coroute::unexpected<std::string>("error");
        REQUIRE(!e.has_value());
        REQUIRE(e.error() == "error");
    }
}

TEST_CASE("expected monadic operations", "[expected]") {
    using coroute::expected;
    
    SECTION("transform") {
        expected<int, std::string> e = 21;
        auto result = e.transform([](int x) { return x * 2; });
        REQUIRE(result.has_value());
        REQUIRE(*result == 42);
    }
    
    SECTION("transform with error") {
        expected<int, std::string> e = coroute::unexpected<std::string>("error");
        auto result = e.transform([](int x) { return x * 2; });
        REQUIRE(!result.has_value());
        REQUIRE(result.error() == "error");
    }
    
    SECTION("and_then") {
        expected<int, std::string> e = 21;
        auto result = e.and_then([](int x) -> expected<int, std::string> {
            return x * 2;
        });
        REQUIRE(result.has_value());
        REQUIRE(*result == 42);
    }
}
