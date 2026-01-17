#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>
#include <coroute/util/from_string.hpp>

using namespace coroute;

TEST_CASE("FromString integral types", "[from_string]") {
    SECTION("int") {
        auto result = from_string<int>("42");
        REQUIRE(result.has_value());
        REQUIRE(*result == 42);
    }
    
    SECTION("negative int") {
        auto result = from_string<int>("-123");
        REQUIRE(result.has_value());
        REQUIRE(*result == -123);
    }
    
    SECTION("size_t") {
        auto result = from_string<size_t>("12345");
        REQUIRE(result.has_value());
        REQUIRE(*result == 12345);
    }
    
    SECTION("invalid int") {
        auto result = from_string<int>("not_a_number");
        REQUIRE(!result.has_value());
    }
    
    SECTION("empty string") {
        auto result = from_string<int>("");
        REQUIRE(!result.has_value());
    }
    
    SECTION("partial number") {
        auto result = from_string<int>("42abc");
        REQUIRE(!result.has_value());
    }
}

TEST_CASE("FromString floating point", "[from_string]") {
    SECTION("double") {
        auto result = from_string<double>("3.14");
        REQUIRE(result.has_value());
        REQUIRE(*result == Catch::Approx(3.14));
    }
    
    SECTION("negative double") {
        auto result = from_string<double>("-2.5");
        REQUIRE(result.has_value());
        REQUIRE(*result == Catch::Approx(-2.5));
    }
    
    SECTION("scientific notation") {
        auto result = from_string<double>("1.5e10");
        REQUIRE(result.has_value());
        REQUIRE(*result == Catch::Approx(1.5e10));
    }
}

TEST_CASE("FromString bool", "[from_string]") {
    SECTION("true values") {
        REQUIRE(*from_string<bool>("true") == true);
        REQUIRE(*from_string<bool>("1") == true);
        REQUIRE(*from_string<bool>("yes") == true);
        REQUIRE(*from_string<bool>("on") == true);
    }
    
    SECTION("false values") {
        REQUIRE(*from_string<bool>("false") == false);
        REQUIRE(*from_string<bool>("0") == false);
        REQUIRE(*from_string<bool>("no") == false);
        REQUIRE(*from_string<bool>("off") == false);
    }
    
    SECTION("invalid bool") {
        auto result = from_string<bool>("maybe");
        REQUIRE(!result.has_value());
    }
}

TEST_CASE("FromString string types", "[from_string]") {
    SECTION("std::string") {
        auto result = from_string<std::string>("hello world");
        REQUIRE(result.has_value());
        REQUIRE(*result == "hello world");
    }
    
    SECTION("std::string_view") {
        auto result = from_string<std::string_view>("hello");
        REQUIRE(result.has_value());
        REQUIRE(*result == "hello");
    }
}

TEST_CASE("FromString optional", "[from_string]") {
    SECTION("present value") {
        auto result = from_string<std::optional<int>>("42");
        REQUIRE(result.has_value());
        REQUIRE(result->has_value());
        REQUIRE(**result == 42);
    }
    
    SECTION("empty string") {
        auto result = from_string<std::optional<int>>("");
        REQUIRE(result.has_value());
        REQUIRE(!result->has_value());
    }
}
