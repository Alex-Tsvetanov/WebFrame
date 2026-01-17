#include <catch2/catch_test_macros.hpp>
#include <coroute/util/object_pool.hpp>
#include <coroute/core/request.hpp>
#include <coroute/core/response.hpp>

using namespace coroute;

TEST_CASE("ObjectPool basic operations", "[pool]") {
    ObjectPool<int> pool(10);
    
    SECTION("Acquire creates new object when pool is empty") {
        auto obj = pool.acquire();
        REQUIRE(obj != nullptr);
        CHECK(pool.size() == 0);
    }
    
    SECTION("Release returns object to pool") {
        auto obj = pool.acquire();
        *obj = 42;
        pool.release(std::move(obj));
        CHECK(pool.size() == 1);
    }
    
    SECTION("Acquire reuses released object") {
        auto obj1 = pool.acquire();
        *obj1 = 42;
        pool.release(std::move(obj1));
        
        auto obj2 = pool.acquire();
        // Object was reused (value preserved since no reset func)
        CHECK(*obj2 == 42);
        CHECK(pool.size() == 0);
    }
    
    SECTION("Pool respects max size") {
        ObjectPool<int> small_pool(2);
        
        auto obj1 = small_pool.acquire();
        auto obj2 = small_pool.acquire();
        auto obj3 = small_pool.acquire();
        
        small_pool.release(std::move(obj1));
        small_pool.release(std::move(obj2));
        small_pool.release(std::move(obj3));  // Should be discarded
        
        CHECK(small_pool.size() == 2);
    }
}

TEST_CASE("ObjectPool with reset function", "[pool]") {
    ObjectPool<std::string> pool(10, [](std::string& s) { s.clear(); });
    
    SECTION("Reset function is called on release") {
        auto obj = pool.acquire();
        *obj = "hello";
        pool.release(std::move(obj));
        
        auto obj2 = pool.acquire();
        CHECK(obj2->empty());  // Was reset
    }
}

TEST_CASE("ObjectPool reserve", "[pool]") {
    ObjectPool<int> pool(100);
    pool.reserve(10);
    CHECK(pool.size() == 10);
}

TEST_CASE("PooledObject RAII", "[pool]") {
    ObjectPool<int> pool(10);
    
    SECTION("Object returned to pool on destruction") {
        {
            auto obj = pool.acquire();
            PooledObject<int> pooled(std::move(obj), &pool);
            *pooled = 42;
        }
        CHECK(pool.size() == 1);
    }
    
    SECTION("Release prevents return to pool") {
        {
            auto obj = pool.acquire();
            PooledObject<int> pooled(std::move(obj), &pool);
            auto released = pooled.release();
            CHECK(released != nullptr);
        }
        CHECK(pool.size() == 0);  // Not returned
    }
    
    SECTION("Move semantics work correctly") {
        PooledObject<int> pooled1(pool.acquire(), &pool);
        *pooled1 = 42;
        
        PooledObject<int> pooled2 = std::move(pooled1);
        CHECK(*pooled2 == 42);
        CHECK(!pooled1);  // Moved from
    }
}

TEST_CASE("BufferPool", "[pool]") {
    BufferPool pool(4096, 10);
    
    SECTION("Acquire returns buffer with reserved capacity") {
        auto buf = pool.acquire();
        CHECK(buf->capacity() >= 4096);
    }
    
    SECTION("Acquire with size returns larger buffer") {
        auto buf = pool.acquire(8192);
        CHECK(buf->capacity() >= 8192);
    }
    
    SECTION("Released buffers are cleared") {
        auto buf = pool.acquire();
        buf->resize(100, 'x');
        pool.release(std::move(buf));
        
        auto buf2 = pool.acquire();
        CHECK(buf2->empty());  // Was cleared
    }
}

TEST_CASE("Request reset", "[pool]") {
    Request req;
    req.set_method(HttpMethod::POST);
    req.set_path("/test");
    req.set_body("body");
    req.add_header("Content-Type", "text/plain");
    req.add_query_param("key", "value");
    
    req.reset();
    
    CHECK(req.method() == HttpMethod::GET);
    CHECK(req.path().empty());
    CHECK(req.body().empty());
    CHECK(req.headers().empty());
}

TEST_CASE("Response reset", "[pool]") {
    Response resp = Response::ok("hello", "text/plain");
    
    resp.reset();
    
    CHECK(resp.status() == 200);
    CHECK(resp.body().empty());
    CHECK(resp.headers().empty());
}

TEST_CASE("Request/Response pool integration", "[pool]") {
    ObjectPool<Request> req_pool(10, [](Request& r) { r.reset(); });
    ObjectPool<Response> resp_pool(10, [](Response& r) { r.reset(); });
    
    // Simulate request handling
    {
        auto req = req_pool.acquire();
        req->set_method(HttpMethod::GET);
        req->set_path("/api/users");
        
        auto resp = resp_pool.acquire();
        resp->set_status(200);
        resp->set_body("[]");
        
        // Return to pools
        req_pool.release(std::move(req));
        resp_pool.release(std::move(resp));
    }
    
    CHECK(req_pool.size() == 1);
    CHECK(resp_pool.size() == 1);
    
    // Reuse
    auto req2 = req_pool.acquire();
    CHECK(req2->path().empty());  // Was reset
    
    auto resp2 = resp_pool.acquire();
    CHECK(resp2->body().empty());  // Was reset
}
