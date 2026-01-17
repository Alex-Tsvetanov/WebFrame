#include <catch2/catch_test_macros.hpp>
#include <coroute/core/error.hpp>

using namespace coroute;

TEST_CASE("Error construction", "[error]") {
    SECTION("IoError") {
        Error e(IoError::Timeout, "Connection timed out");
        REQUIRE(e.is_io());
        REQUIRE(!e.is_http());
        REQUIRE(!e.is_system());
        REQUIRE(e.io_error() == IoError::Timeout);
        REQUIRE(e.message() == "Connection timed out");
    }
    
    SECTION("HttpError") {
        Error e(HttpError::NotFound, "Page not found");
        REQUIRE(!e.is_io());
        REQUIRE(e.is_http());
        REQUIRE(!e.is_system());
        REQUIRE(e.http_error() == HttpError::NotFound);
        REQUIRE(e.http_status() == 404);
    }
    
    SECTION("system error") {
        auto ec = std::make_error_code(std::errc::no_such_file_or_directory);
        Error e(ec);
        REQUIRE(!e.is_io());
        REQUIRE(!e.is_http());
        REQUIRE(e.is_system());
    }
}

TEST_CASE("Error factory methods", "[error]") {
    SECTION("cancelled") {
        Error e = Error::cancelled();
        REQUIRE(e.is_cancelled());
        REQUIRE(e.io_error() == IoError::Cancelled);
    }
    
    SECTION("timeout") {
        Error e = Error::timeout();
        REQUIRE(e.is_timeout());
        REQUIRE(e.io_error() == IoError::Timeout);
    }
    
    SECTION("http") {
        Error e = Error::http(HttpError::BadRequest, "Invalid input");
        REQUIRE(e.is_http());
        REQUIRE(e.http_status() == 400);
    }
    
    SECTION("http from int") {
        Error e = Error::http(404, "Not found");
        REQUIRE(e.is_http());
        REQUIRE(e.http_status() == 404);
    }
}

TEST_CASE("Error to_string", "[error]") {
    SECTION("IoError") {
        Error e(IoError::ConnectionReset, "Connection was reset");
        std::string s = e.to_string();
        REQUIRE(s.find("Connection reset") != std::string::npos);
        REQUIRE(s.find("Connection was reset") != std::string::npos);
    }
    
    SECTION("HttpError") {
        Error e(HttpError::NotFound);
        std::string s = e.to_string();
        REQUIRE(s.find("404") != std::string::npos);
        REQUIRE(s.find("Not Found") != std::string::npos);
    }
}

TEST_CASE("Error http_status mapping", "[error]") {
    SECTION("IoError to HTTP status") {
        REQUIRE(Error(IoError::Timeout).http_status() == 408);
        REQUIRE(Error(IoError::Cancelled).http_status() == 499);
        REQUIRE(Error(IoError::ConnectionReset).http_status() == 500);
    }
    
    SECTION("HttpError to HTTP status") {
        REQUIRE(Error(HttpError::BadRequest).http_status() == 400);
        REQUIRE(Error(HttpError::NotFound).http_status() == 404);
        REQUIRE(Error(HttpError::Internal).http_status() == 500);
    }
}
