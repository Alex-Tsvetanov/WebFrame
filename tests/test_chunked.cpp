#include <catch2/catch_test_macros.hpp>
#include <coroute/core/chunked.hpp>

using namespace coroute;

TEST_CASE("Chunked encoding utilities", "[chunked]") {
    SECTION("encode_chunk creates valid chunk format") {
        auto chunk = chunked::encode_chunk("Hello");
        CHECK(chunk == "5\r\nHello\r\n");
        
        chunk = chunked::encode_chunk("Hello, World!");
        CHECK(chunk == "d\r\nHello, World!\r\n");  // 13 = 0xd
    }
    
    SECTION("encode_chunk handles empty data") {
        auto chunk = chunked::encode_chunk("");
        CHECK(chunk.empty());
    }
    
    SECTION("encode_final_chunk creates terminator") {
        auto final_chunk = chunked::encode_final_chunk();
        CHECK(final_chunk == "0\r\n\r\n");
    }
    
    SECTION("encode_final_chunk with trailers") {
        std::vector<std::pair<std::string, std::string>> trailers = {
            {"X-Checksum", "abc123"},
            {"X-Count", "42"}
        };
        auto final_chunk = chunked::encode_final_chunk(trailers);
        CHECK(final_chunk == "0\r\nX-Checksum: abc123\r\nX-Count: 42\r\n\r\n");
    }
}

TEST_CASE("Chunked size parsing", "[chunked]") {
    SECTION("parse_chunk_size handles hex values") {
        CHECK(chunked::parse_chunk_size("0") == 0);
        CHECK(chunked::parse_chunk_size("5") == 5);
        CHECK(chunked::parse_chunk_size("a") == 10);
        CHECK(chunked::parse_chunk_size("A") == 10);
        CHECK(chunked::parse_chunk_size("f") == 15);
        CHECK(chunked::parse_chunk_size("F") == 15);
        CHECK(chunked::parse_chunk_size("10") == 16);
        CHECK(chunked::parse_chunk_size("ff") == 255);
        CHECK(chunked::parse_chunk_size("FF") == 255);
        CHECK(chunked::parse_chunk_size("100") == 256);
        CHECK(chunked::parse_chunk_size("1000") == 4096);
    }
    
    SECTION("parse_chunk_size handles whitespace") {
        CHECK(chunked::parse_chunk_size("  5  ") == 5);
        CHECK(chunked::parse_chunk_size("\t10\t") == 16);
    }
    
    SECTION("parse_chunk_size handles chunk extensions") {
        CHECK(chunked::parse_chunk_size("5;name=value") == 5);
        CHECK(chunked::parse_chunk_size("a;ext1;ext2") == 10);
        CHECK(chunked::parse_chunk_size("ff;foo=bar;baz") == 255);
    }
    
    SECTION("parse_chunk_size returns -1 for invalid input") {
        CHECK(chunked::parse_chunk_size("") == -1);
        CHECK(chunked::parse_chunk_size("g") == -1);  // Invalid hex
        CHECK(chunked::parse_chunk_size("xyz") == -1);
        CHECK(chunked::parse_chunk_size("-1") == -1);
    }
}

TEST_CASE("ChunkedResponse configuration", "[chunked]") {
    ChunkedResponse resp;
    
    SECTION("Initial state") {
        CHECK_FALSE(resp.headers_sent());
        CHECK_FALSE(resp.finished());
    }
    
    SECTION("Fluent API") {
        resp.status(201)
            .header("X-Custom", "value")
            .content_type("application/json")
            .trailer("X-Checksum", "abc");
        
        // Can't easily verify without connection, but at least it compiles
        CHECK_FALSE(resp.headers_sent());
    }
}
