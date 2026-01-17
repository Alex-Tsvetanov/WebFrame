#include <catch2/catch_test_macros.hpp>
#include <coroute/core/router.hpp>

using namespace coroute;

TEST_CASE("Router basic matching", "[router]") {
    Router router;
    
    bool handler_called = false;
    router.get("/hello", [&handler_called](Request& req) -> Task<Response> {
        handler_called = true;
        co_return Response::ok("Hello");
    });
    
    SECTION("exact match") {
        auto match = router.match(HttpMethod::GET, "/hello");
        REQUIRE(match);
        REQUIRE(match.handler != nullptr);
        REQUIRE(match.params.empty());
    }
    
    SECTION("no match - different path") {
        auto match = router.match(HttpMethod::GET, "/world");
        REQUIRE(!match);
    }
    
    SECTION("no match - different method") {
        auto match = router.match(HttpMethod::POST, "/hello");
        REQUIRE(!match);
    }
}

TEST_CASE("Router parameter extraction", "[router]") {
    Router router;
    
    router.get("/user/{id}", [](Request& req) -> Task<Response> {
        co_return Response::ok();
    });
    
    router.get("/user/{uid}/post/{pid}", [](Request& req) -> Task<Response> {
        co_return Response::ok();
    });
    
    SECTION("single parameter") {
        auto match = router.match(HttpMethod::GET, "/user/123");
        REQUIRE(match);
        REQUIRE(match.params.size() == 1);
        REQUIRE(match.params[0] == "123");
    }
    
    SECTION("multiple parameters") {
        auto match = router.match(HttpMethod::GET, "/user/42/post/99");
        REQUIRE(match);
        REQUIRE(match.params.size() == 2);
        REQUIRE(match.params[0] == "42");
        REQUIRE(match.params[1] == "99");
    }
    
    SECTION("string parameter") {
        auto match = router.match(HttpMethod::GET, "/user/john_doe");
        REQUIRE(match);
        REQUIRE(match.params.size() == 1);
        REQUIRE(match.params[0] == "john_doe");
    }
}

TEST_CASE("Router HTTP methods", "[router]") {
    Router router;
    
    router.get("/resource", [](Request& req) -> Task<Response> {
        co_return Response::ok("GET");
    });
    
    router.post("/resource", [](Request& req) -> Task<Response> {
        co_return Response::ok("POST");
    });
    
    router.put("/resource", [](Request& req) -> Task<Response> {
        co_return Response::ok("PUT");
    });
    
    router.del("/resource", [](Request& req) -> Task<Response> {
        co_return Response::ok("DELETE");
    });
    
    SECTION("GET") {
        auto match = router.match(HttpMethod::GET, "/resource");
        REQUIRE(match);
    }
    
    SECTION("POST") {
        auto match = router.match(HttpMethod::POST, "/resource");
        REQUIRE(match);
    }
    
    SECTION("PUT") {
        auto match = router.match(HttpMethod::PUT, "/resource");
        REQUIRE(match);
    }
    
    SECTION("DELETE") {
        auto match = router.match(HttpMethod::DELETE, "/resource");
        REQUIRE(match);
    }
    
    SECTION("PATCH - not registered") {
        auto match = router.match(HttpMethod::PATCH, "/resource");
        REQUIRE(!match);
    }
}
