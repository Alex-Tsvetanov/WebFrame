#include <catch2/catch_test_macros.hpp>
#include <coroute/core/compression.hpp>

using namespace coroute;

TEST_CASE("Compression algorithm names", "[compression]") {
    CHECK(algorithm_name(CompressionAlgorithm::Gzip) == "gzip");
    CHECK(algorithm_name(CompressionAlgorithm::Deflate) == "deflate");
    CHECK(algorithm_name(CompressionAlgorithm::Brotli) == "br");
    CHECK(algorithm_name(CompressionAlgorithm::Identity) == "identity");
}

TEST_CASE("Accept-Encoding parsing", "[compression]") {
    std::set<CompressionAlgorithm> all_algos = {
        CompressionAlgorithm::Gzip,
        CompressionAlgorithm::Deflate
    };
    
    SECTION("Simple encoding") {
        CHECK(parse_accept_encoding("gzip", all_algos) == CompressionAlgorithm::Gzip);
        CHECK(parse_accept_encoding("deflate", all_algos) == CompressionAlgorithm::Deflate);
    }
    
    SECTION("Multiple encodings") {
        // Should prefer gzip over deflate (our preference order)
        auto algo = parse_accept_encoding("deflate, gzip", all_algos);
        CHECK(algo == CompressionAlgorithm::Gzip);
    }
    
    SECTION("Quality values") {
        // gzip with higher quality should be preferred
        auto algo = parse_accept_encoding("deflate;q=0.5, gzip;q=1.0", all_algos);
        CHECK(algo == CompressionAlgorithm::Gzip);
        
        // deflate with higher quality should be preferred
        algo = parse_accept_encoding("gzip;q=0.5, deflate;q=1.0", all_algos);
        CHECK(algo == CompressionAlgorithm::Deflate);
    }
    
    SECTION("Zero quality means disabled") {
        auto algo = parse_accept_encoding("gzip;q=0, deflate", all_algos);
        CHECK(algo == CompressionAlgorithm::Deflate);
    }
    
    SECTION("Unknown encoding returns Identity") {
        std::set<CompressionAlgorithm> empty;
        CHECK(parse_accept_encoding("gzip", empty) == CompressionAlgorithm::Identity);
        CHECK(parse_accept_encoding("unknown", all_algos) == CompressionAlgorithm::Identity);
    }
    
    SECTION("Empty header returns Identity") {
        CHECK(parse_accept_encoding("", all_algos) == CompressionAlgorithm::Identity);
    }
    
    SECTION("Case insensitive") {
        CHECK(parse_accept_encoding("GZIP", all_algos) == CompressionAlgorithm::Gzip);
        CHECK(parse_accept_encoding("GzIp", all_algos) == CompressionAlgorithm::Gzip);
    }
    
    SECTION("x-gzip alias") {
        CHECK(parse_accept_encoding("x-gzip", all_algos) == CompressionAlgorithm::Gzip);
    }
}

TEST_CASE("Gzip compression", "[compression]") {
    SECTION("Compress and decompress") {
        std::string original = "Hello, World! This is a test string for compression.";
        
        auto compressed = compress::gzip(original);
        REQUIRE(compressed.has_value());
        CHECK(compressed->size() > 0);
        
        auto decompressed = compress::gunzip(*compressed);
        REQUIRE(decompressed.has_value());
        CHECK(*decompressed == original);
    }
    
    SECTION("Empty data") {
        auto compressed = compress::gzip("");
        REQUIRE(compressed.has_value());
        CHECK(compressed->empty());
    }
    
    SECTION("Large data") {
        std::string large(100000, 'x');  // 100KB of repeated chars
        
        auto compressed = compress::gzip(large);
        REQUIRE(compressed.has_value());
        // Repeated data should compress very well
        CHECK(compressed->size() < large.size() / 10);
        
        auto decompressed = compress::gunzip(*compressed);
        REQUIRE(decompressed.has_value());
        CHECK(*decompressed == large);
    }
    
    SECTION("Different compression levels") {
        std::string data = "This is test data that will be compressed at different levels.";
        data += data + data + data;  // Make it bigger
        
        auto level1 = compress::gzip(data, 1);
        auto level9 = compress::gzip(data, 9);
        
        REQUIRE(level1.has_value());
        REQUIRE(level9.has_value());
        
        // Higher level should produce smaller output (usually)
        // Both should decompress to original
        auto dec1 = compress::gunzip(*level1);
        auto dec9 = compress::gunzip(*level9);
        
        REQUIRE(dec1.has_value());
        REQUIRE(dec9.has_value());
        CHECK(*dec1 == data);
        CHECK(*dec9 == data);
    }
}

TEST_CASE("Deflate compression", "[compression]") {
    SECTION("Compress and decompress") {
        std::string original = "Hello, World! This is a test string for deflate compression.";
        
        auto compressed = compress::deflate(original);
        REQUIRE(compressed.has_value());
        CHECK(compressed->size() > 0);
        
        auto decompressed = compress::inflate(*compressed);
        REQUIRE(decompressed.has_value());
        CHECK(*decompressed == original);
    }
    
    SECTION("Empty data") {
        auto compressed = compress::deflate("");
        REQUIRE(compressed.has_value());
        CHECK(compressed->empty());
    }
}

TEST_CASE("Brotli availability", "[compression]") {
    // Just check that the function doesn't crash
    bool available = compress::brotli_available();
    
    if (available) {
        SECTION("Brotli compress and decompress") {
            std::string original = "Hello, World! This is a test string for Brotli compression.";
            
            auto compressed = compress::brotli(original);
            REQUIRE(compressed.has_value());
            
            auto decompressed = compress::brotli_decompress(*compressed);
            REQUIRE(decompressed.has_value());
            CHECK(*decompressed == original);
        }
    } else {
        SECTION("Brotli returns nullopt when not available") {
            auto result = compress::brotli("test");
            CHECK_FALSE(result.has_value());
        }
    }
}

TEST_CASE("CompressionOptions defaults", "[compression]") {
    CompressionOptions options;
    
    CHECK(options.min_size == 1024);
    CHECK(options.level == 6);
    CHECK(options.skip_if_encoded == true);
    CHECK(options.add_vary_header == true);
    CHECK(options.compressible_types.size() > 0);
}

TEST_CASE("CompressionMiddleware content type matching", "[compression]") {
    CompressionMiddleware middleware;
    
    SECTION("Exact matches") {
        CHECK(middleware.should_compress("text/html"));
        CHECK(middleware.should_compress("application/json"));
        CHECK(middleware.should_compress("text/css"));
    }
    
    SECTION("With charset") {
        CHECK(middleware.should_compress("text/html; charset=utf-8"));
        CHECK(middleware.should_compress("application/json; charset=utf-8"));
    }
    
    SECTION("Non-compressible types") {
        CHECK_FALSE(middleware.should_compress("image/png"));
        CHECK_FALSE(middleware.should_compress("image/jpeg"));
        CHECK_FALSE(middleware.should_compress("video/mp4"));
    }
}

TEST_CASE("Generic compress function", "[compression]") {
    std::string data = "Test data for compression";
    
    SECTION("Gzip via generic function") {
        auto result = compress::compress(data, CompressionAlgorithm::Gzip);
        REQUIRE(result.has_value());
        
        auto decompressed = compress::gunzip(*result);
        REQUIRE(decompressed.has_value());
        CHECK(*decompressed == data);
    }
    
    SECTION("Deflate via generic function") {
        auto result = compress::compress(data, CompressionAlgorithm::Deflate);
        REQUIRE(result.has_value());
        
        auto decompressed = compress::inflate(*result);
        REQUIRE(decompressed.has_value());
        CHECK(*decompressed == data);
    }
    
    SECTION("Identity returns original") {
        auto result = compress::compress(data, CompressionAlgorithm::Identity);
        REQUIRE(result.has_value());
        CHECK(*result == data);
    }
}
