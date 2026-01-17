// test_auth_state.cpp - Unit tests for authentication state propagation

#include "coroute/core/auth_state.hpp"
#include "coroute/core/request.hpp"
#include "coroute/core/response.hpp"
#include <catch2/catch_test_macros.hpp>

using namespace coroute;

// ============================================================================
// WebAuthState Tests
// ============================================================================

TEST_CASE("WebAuthState - is a no-op for all operations", "[auth][web]") {
  WebAuthState auth;

  SECTION("apply does not modify request") {
    Request req;
    req.set_path("/test");
    auth.apply(req);
    // No assertions needed - just verifying no crash
    CHECK(req.path() == "/test");
  }

  SECTION("observe does not crash") {
    Response resp = Response::ok("test");
    auth.observe(resp);
    // No assertions needed
  }

  SECTION("authenticated returns false") {
    CHECK(auth.authenticated() == false);
  }

  SECTION("clear does not crash") { auth.clear(); }
}

// ============================================================================
// ClientAuthState Tests
// ============================================================================

TEST_CASE("ClientAuthState - cookie storage and retrieval", "[auth][client]") {
  ClientAuthState auth;

  SECTION("initially not authenticated") {
    CHECK(auth.authenticated() == false);
    CHECK(auth.cookies().empty());
    CHECK(auth.bearer_token().has_value() == false);
  }

  SECTION("set and get cookie") {
    auth.set_cookie("session_id", "abc123");
    CHECK(auth.authenticated() == true);
    CHECK(auth.get_cookie("session_id").value() == "abc123");
    CHECK(auth.get_cookie("nonexistent").has_value() == false);
  }

  SECTION("clear removes all state") {
    auth.set_cookie("session_id", "abc123");
    auth.set_bearer_token("my_token");
    CHECK(auth.authenticated() == true);

    auth.clear();
    CHECK(auth.authenticated() == false);
    CHECK(auth.cookies().empty());
    CHECK(auth.bearer_token().has_value() == false);
  }
}

TEST_CASE("ClientAuthState - bearer token management", "[auth][client]") {
  ClientAuthState auth;

  SECTION("set and get bearer token") {
    CHECK(auth.bearer_token().has_value() == false);

    auth.set_bearer_token("test_token_123");
    CHECK(auth.authenticated() == true);
    CHECK(auth.bearer_token().value() == "test_token_123");
  }
}

TEST_CASE("ClientAuthState - apply adds auth to request", "[auth][client]") {
  ClientAuthState auth;

  SECTION("adds Cookie header") {
    auth.set_cookie("session_id", "abc123");
    auth.set_cookie("user", "alice");

    Request req;
    auth.apply(req);

    auto cookie = req.header("Cookie");
    REQUIRE(cookie.has_value());
    // Order may vary, so check for both cookies
    CHECK((cookie->find("session_id=abc123") != std::string::npos));
    CHECK((cookie->find("user=alice") != std::string::npos));
  }

  SECTION("adds Authorization header") {
    auth.set_bearer_token("my_jwt_token");

    Request req;
    auth.apply(req);

    auto auth_header = req.header("Authorization");
    REQUIRE(auth_header.has_value());
    CHECK(*auth_header == "Bearer my_jwt_token");
  }

  SECTION("adds both Cookie and Authorization headers") {
    auth.set_cookie("session", "xyz");
    auth.set_bearer_token("token123");

    Request req;
    auth.apply(req);

    CHECK(req.header("Cookie").has_value());
    CHECK(req.header("Authorization").has_value());
  }
}

TEST_CASE("ClientAuthState - observe learns from Set-Cookie",
          "[auth][client]") {
  ClientAuthState auth;

  SECTION("parses simple Set-Cookie") {
    Response resp = Response::ok();
    resp.add_header("Set-Cookie", "session_id=new_session");

    auth.observe(resp);

    CHECK(auth.authenticated() == true);
    CHECK(auth.get_cookie("session_id").value() == "new_session");
  }

  SECTION("parses Set-Cookie with attributes") {
    Response resp = Response::ok();
    resp.add_header("Set-Cookie", "token=abc123; HttpOnly; Secure; Path=/");

    auth.observe(resp);

    // Should extract just the name=value part
    CHECK(auth.get_cookie("token").value() == "abc123");
  }

  SECTION("handles multiple Set-Cookie headers") {
    Response resp = Response::ok();
    resp.add_header("Set-Cookie", "cookie1=value1");
    resp.add_header("Set-Cookie", "cookie2=value2");

    auth.observe(resp);

    CHECK(auth.get_cookie("cookie1").value() == "value1");
    CHECK(auth.get_cookie("cookie2").value() == "value2");
  }

  SECTION("overwrites existing cookie") {
    auth.set_cookie("session", "old_value");

    Response resp = Response::ok();
    resp.add_header("Set-Cookie", "session=new_value");

    auth.observe(resp);

    CHECK(auth.get_cookie("session").value() == "new_value");
  }
}

// ============================================================================
// Auth Propagation Flow Tests
// ============================================================================

TEST_CASE("Auth propagation - full apply/observe cycle", "[auth][client]") {
  ClientAuthState auth;

  // Step 1: No auth initially
  CHECK(auth.authenticated() == false);

  // Step 2: Login response sets cookie
  Response login_resp = Response::ok(R"({"status":"logged_in"})");
  login_resp.add_header("Set-Cookie", "auth_token=jwt_here; HttpOnly");

  auth.observe(login_resp);
  CHECK(auth.authenticated() == true);

  // Step 3: Subsequent request includes cookie
  Request api_req;
  api_req.set_path("/api/data");
  auth.apply(api_req);

  auto cookie = api_req.header("Cookie");
  REQUIRE(cookie.has_value());
  CHECK(cookie->find("auth_token=jwt_here") != std::string::npos);
}
