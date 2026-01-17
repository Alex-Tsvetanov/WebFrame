#include <catch2/catch_test_macros.hpp>
#include <coroute/core/response.hpp>

using namespace coroute;

TEST_CASE("Response factory methods", "[response]") {
    SECTION("ok") {
        auto resp = Response::ok("Hello");
        REQUIRE(resp.status() == 200);
        REQUIRE(resp.body() == "Hello");
    }
    
    SECTION("json") {
        auto resp = Response::json(R"({"key": "value"})");
        REQUIRE(resp.status() == 200);
        
        // Check Content-Type header
        bool found_content_type = false;
        for (const auto& [k, v] : resp.headers()) {
            if (k == "Content-Type" && v == "application/json") {
                found_content_type = true;
                break;
            }
        }
        REQUIRE(found_content_type);
    }
    
    SECTION("not_found") {
        auto resp = Response::not_found();
        REQUIRE(resp.status() == 404);
    }
    
    SECTION("bad_request") {
        auto resp = Response::bad_request("Invalid input");
        REQUIRE(resp.status() == 400);
        REQUIRE(resp.body() == "Invalid input");
    }
    
    SECTION("internal_error") {
        auto resp = Response::internal_error();
        REQUIRE(resp.status() == 500);
    }
    
    SECTION("redirect") {
        auto resp = Response::redirect("/new-location");
        REQUIRE(resp.status() == 302);
        
        bool found_location = false;
        for (const auto& [k, v] : resp.headers()) {
            if (k == "Location" && v == "/new-location") {
                found_location = true;
                break;
            }
        }
        REQUIRE(found_location);
    }
}

TEST_CASE("Response serialization", "[response]") {
    SECTION("basic response") {
        auto resp = Response::ok("Hello, World!");
        std::string serialized = resp.serialize();
        
        REQUIRE(serialized.find("HTTP/1.1 200 OK") != std::string::npos);
        REQUIRE(serialized.find("Content-Length: 13") != std::string::npos);
        REQUIRE(serialized.find("\r\n\r\n") != std::string::npos);
        REQUIRE(serialized.find("Hello, World!") != std::string::npos);
    }
}

TEST_CASE("ResponseBuilder", "[response]") {
    SECTION("basic building") {
        auto resp = ResponseBuilder()
            .status(201)
            .header("X-Custom", "value")
            .body("Created")
            .build();
        
        REQUIRE(resp.status() == 201);
        REQUIRE(resp.body() == "Created");
        
        bool found_custom = false;
        for (const auto& [k, v] : resp.headers()) {
            if (k == "X-Custom" && v == "value") {
                found_custom = true;
                break;
            }
        }
        REQUIRE(found_custom);
    }
    
    SECTION("content_type helper") {
        auto resp = ResponseBuilder()
            .content_type("text/html")
            .body("<h1>Hello</h1>")
            .build();
        
        bool found_ct = false;
        for (const auto& [k, v] : resp.headers()) {
            if (k == "Content-Type" && v == "text/html") {
                found_ct = true;
                break;
            }
        }
        REQUIRE(found_ct);
    }
    
    SECTION("json_body helper") {
        auto resp = ResponseBuilder()
            .json_body(R"({"status": "ok"})")
            .build();
        
        bool found_json_ct = false;
        for (const auto& [k, v] : resp.headers()) {
            if (k == "Content-Type" && v == "application/json") {
                found_json_ct = true;
                break;
            }
        }
        REQUIRE(found_json_ct);
    }
    
    SECTION("auto Content-Length") {
        auto resp = ResponseBuilder()
            .body("12345")
            .build();
        
        bool found_cl = false;
        for (const auto& [k, v] : resp.headers()) {
            if (k == "Content-Length" && v == "5") {
                found_cl = true;
                break;
            }
        }
        REQUIRE(found_cl);
    }
}
