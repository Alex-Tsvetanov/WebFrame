#include <catch2/catch_test_macros.hpp>
#include <coroute/core/range.hpp>

using namespace coroute;

TEST_CASE("ByteRange normalization", "[range]") {
    SECTION("Normal range 0-499 in 1000 byte file") {
        ByteRange range;
        range.start = 0;
        range.end = 499;
        
        REQUIRE(range.normalize(1000));
        CHECK(range.get_start() == 0);
        CHECK(range.get_end() == 499);
        CHECK(range.length() == 500);
    }
    
    SECTION("Open-ended range 500- in 1000 byte file") {
        ByteRange range;
        range.start = 500;
        // end is nullopt
        
        REQUIRE(range.normalize(1000));
        CHECK(range.get_start() == 500);
        CHECK(range.get_end() == 999);
        CHECK(range.length() == 500);
    }
    
    SECTION("Suffix range -500 in 1000 byte file") {
        ByteRange range;
        // start is nullopt
        range.end = 500;
        
        REQUIRE(range.normalize(1000));
        CHECK(range.get_start() == 500);
        CHECK(range.get_end() == 999);
        CHECK(range.length() == 500);
    }
    
    SECTION("Suffix range larger than file") {
        ByteRange range;
        range.end = 2000;  // Suffix of 2000 bytes
        
        REQUIRE(range.normalize(1000));
        CHECK(range.get_start() == 0);
        CHECK(range.get_end() == 999);
        CHECK(range.length() == 1000);
    }
    
    SECTION("End clamped to file size") {
        ByteRange range;
        range.start = 500;
        range.end = 2000;  // Beyond file size
        
        REQUIRE(range.normalize(1000));
        CHECK(range.get_start() == 500);
        CHECK(range.get_end() == 999);
    }
    
    SECTION("Start beyond file size is invalid") {
        ByteRange range;
        range.start = 1500;
        range.end = 2000;
        
        CHECK_FALSE(range.normalize(1000));
    }
    
    SECTION("End before start is invalid") {
        ByteRange range;
        range.start = 500;
        range.end = 100;
        
        CHECK_FALSE(range.normalize(1000));
    }
    
    SECTION("Zero-length file") {
        ByteRange range;
        range.start = 0;
        range.end = 0;
        
        CHECK_FALSE(range.normalize(0));
    }
}

TEST_CASE("ByteRange Content-Range formatting", "[range]") {
    ByteRange range;
    range.start = 0;
    range.end = 499;
    
    CHECK(range.to_content_range(1000) == "bytes 0-499/1000");
    
    range.start = 500;
    range.end = 999;
    CHECK(range.to_content_range(1000) == "bytes 500-999/1000");
}

TEST_CASE("Range header parsing", "[range]") {
    SECTION("Single range") {
        auto result = range::parse("bytes=0-499");
        REQUIRE(result.has_value());
        CHECK(result->unit == "bytes");
        REQUIRE(result->ranges.size() == 1);
        CHECK(result->ranges[0].start == 0);
        CHECK(result->ranges[0].end == 499);
    }
    
    SECTION("Open-ended range") {
        auto result = range::parse("bytes=500-");
        REQUIRE(result.has_value());
        REQUIRE(result->ranges.size() == 1);
        CHECK(result->ranges[0].start == 500);
        CHECK_FALSE(result->ranges[0].end.has_value());
    }
    
    SECTION("Suffix range") {
        auto result = range::parse("bytes=-500");
        REQUIRE(result.has_value());
        REQUIRE(result->ranges.size() == 1);
        CHECK_FALSE(result->ranges[0].start.has_value());
        CHECK(result->ranges[0].end == 500);
    }
    
    SECTION("Multiple ranges") {
        auto result = range::parse("bytes=0-99, 200-299, -100");
        REQUIRE(result.has_value());
        REQUIRE(result->ranges.size() == 3);
        
        CHECK(result->ranges[0].start == 0);
        CHECK(result->ranges[0].end == 99);
        
        CHECK(result->ranges[1].start == 200);
        CHECK(result->ranges[1].end == 299);
        
        CHECK_FALSE(result->ranges[2].start.has_value());
        CHECK(result->ranges[2].end == 100);
    }
    
    SECTION("With whitespace") {
        auto result = range::parse("bytes = 0 - 499");
        REQUIRE(result.has_value());
        REQUIRE(result->ranges.size() == 1);
        CHECK(result->ranges[0].start == 0);
        CHECK(result->ranges[0].end == 499);
    }
    
    SECTION("Invalid - no equals sign") {
        auto result = range::parse("bytes 0-499");
        CHECK_FALSE(result.has_value());
    }
    
    SECTION("Invalid - unsupported unit") {
        auto result = range::parse("items=0-499");
        CHECK_FALSE(result.has_value());
    }
    
    SECTION("Invalid - empty") {
        auto result = range::parse("");
        CHECK_FALSE(result.has_value());
    }
    
    SECTION("Invalid - no ranges") {
        auto result = range::parse("bytes=");
        CHECK_FALSE(result.has_value());
    }
}

TEST_CASE("RangeResponseBuilder with content", "[range]") {
    std::string content = "0123456789";  // 10 bytes
    
    SECTION("Full response without Range header") {
        Request req;
        // No Range header
        
        RangeResponseBuilder builder;
        builder.content(content, "text/plain");
        
        Response resp = builder.build(req);
        CHECK(resp.status() == 200);
        CHECK(resp.body() == content);
    }
    
    SECTION("Partial response with Range header") {
        Request req;
        req.add_header("Range", "bytes=0-4");
        
        RangeResponseBuilder builder;
        builder.content(content, "text/plain");
        
        Response resp = builder.build(req);
        CHECK(resp.status() == 206);
        CHECK(resp.body() == "01234");
        
        // Check Content-Range header
        bool found_content_range = false;
        for (const auto& [key, value] : resp.headers()) {
            if (key == "Content-Range") {
                CHECK(value == "bytes 0-4/10");
                found_content_range = true;
            }
        }
        CHECK(found_content_range);
    }
    
    SECTION("Suffix range") {
        Request req;
        req.add_header("Range", "bytes=-3");
        
        RangeResponseBuilder builder;
        builder.content(content, "text/plain");
        
        Response resp = builder.build(req);
        CHECK(resp.status() == 206);
        CHECK(resp.body() == "789");
    }
    
    SECTION("Open-ended range") {
        Request req;
        req.add_header("Range", "bytes=7-");
        
        RangeResponseBuilder builder;
        builder.content(content, "text/plain");
        
        Response resp = builder.build(req);
        CHECK(resp.status() == 206);
        CHECK(resp.body() == "789");
    }
    
    SECTION("Invalid range returns 416") {
        Request req;
        req.add_header("Range", "bytes=100-200");  // Beyond content
        
        RangeResponseBuilder builder;
        builder.content(content, "text/plain");
        
        Response resp = builder.build(req);
        CHECK(resp.status() == 416);
    }
}

TEST_CASE("RangeResponseBuilder with If-Range", "[range]") {
    std::string content = "0123456789";
    std::string etag = "\"abc123\"";
    
    SECTION("If-Range matches ETag - serve range") {
        Request req;
        req.add_header("Range", "bytes=0-4");
        req.add_header("If-Range", "\"abc123\"");
        
        RangeResponseBuilder builder;
        builder.content(content, "text/plain").etag(etag);
        
        Response resp = builder.build(req);
        CHECK(resp.status() == 206);
        CHECK(resp.body() == "01234");
    }
    
    SECTION("If-Range doesn't match - serve full content") {
        Request req;
        req.add_header("Range", "bytes=0-4");
        req.add_header("If-Range", "\"different\"");
        
        RangeResponseBuilder builder;
        builder.content(content, "text/plain").etag(etag);
        
        Response resp = builder.build(req);
        CHECK(resp.status() == 200);
        CHECK(resp.body() == content);
    }
}

TEST_CASE("Convenience functions", "[range]") {
    SECTION("partial_content") {
        Response resp = partial_content("Hello", 0, 4, 10, "text/plain");
        CHECK(resp.status() == 206);
        CHECK(resp.body() == "Hello");
        
        bool found = false;
        for (const auto& [key, value] : resp.headers()) {
            if (key == "Content-Range") {
                CHECK(value == "bytes 0-4/10");
                found = true;
            }
        }
        CHECK(found);
    }
    
    SECTION("range_not_satisfiable") {
        Response resp = range_not_satisfiable(1000);
        CHECK(resp.status() == 416);
        
        bool found = false;
        for (const auto& [key, value] : resp.headers()) {
            if (key == "Content-Range") {
                CHECK(value == "bytes */1000");
                found = true;
            }
        }
        CHECK(found);
    }
}

TEST_CASE("RangeHeader validation", "[range]") {
    SECTION("Valid single range") {
        auto header = range::parse("bytes=0-499");
        REQUIRE(header.has_value());
        CHECK(header->is_valid());
        CHECK(header->is_single_range());
    }
    
    SECTION("Valid multiple ranges") {
        auto header = range::parse("bytes=0-99,200-299");
        REQUIRE(header.has_value());
        CHECK(header->is_valid());
        CHECK_FALSE(header->is_single_range());
    }
}
