#include <catch2/catch_test_macros.hpp>
#include <coroute/core/static_files.hpp>
#include <filesystem>
#include <fstream>

using namespace coroute;

TEST_CASE("MimeTypes returns correct types", "[static_files]") {
    SECTION("Common web types") {
        CHECK(MimeTypes::get("html") == "text/html");
        CHECK(MimeTypes::get("css") == "text/css");
        CHECK(MimeTypes::get("js") == "text/javascript");
        CHECK(MimeTypes::get("json") == "application/json");
    }
    
    SECTION("Image types") {
        CHECK(MimeTypes::get("png") == "image/png");
        CHECK(MimeTypes::get("jpg") == "image/jpeg");
        CHECK(MimeTypes::get("jpeg") == "image/jpeg");
        CHECK(MimeTypes::get("gif") == "image/gif");
        CHECK(MimeTypes::get("svg") == "image/svg+xml");
        CHECK(MimeTypes::get("webp") == "image/webp");
    }
    
    SECTION("Case insensitive") {
        CHECK(MimeTypes::get("HTML") == "text/html");
        CHECK(MimeTypes::get("CSS") == "text/css");
        CHECK(MimeTypes::get("PNG") == "image/png");
    }
    
    SECTION("Unknown extension returns octet-stream") {
        CHECK(MimeTypes::get("xyz") == "application/octet-stream");
        CHECK(MimeTypes::get("unknown") == "application/octet-stream");
    }
    
    SECTION("From path") {
        CHECK(MimeTypes::from_path("file.html") == "text/html");
        CHECK(MimeTypes::from_path("/path/to/style.css") == "text/css");
        CHECK(MimeTypes::from_path("app.min.js") == "text/javascript");
        CHECK(MimeTypes::from_path("noextension") == "application/octet-stream");
    }
}

TEST_CASE("MimeTypes custom registration", "[static_files]") {
    MimeTypes::register_type("custom", "application/x-custom");
    CHECK(MimeTypes::get("custom") == "application/x-custom");
    CHECK(MimeTypes::from_path("file.custom") == "application/x-custom");
}

TEST_CASE("StaticFileOptions defaults", "[static_files]") {
    StaticFileOptions options;
    
    CHECK(options.index_file == "index.html");
    CHECK(options.directory_listing == false);
    CHECK(options.etag == true);
    CHECK(options.last_modified == true);
    CHECK(options.max_age_seconds == 0);
    CHECK(options.immutable == false);
}

TEST_CASE("StaticFileServer path security", "[static_files]") {
    // Create a temporary directory structure
    auto temp_dir = std::filesystem::temp_directory_path() / "coroute_test";
    std::filesystem::create_directories(temp_dir);
    std::filesystem::create_directories(temp_dir / "subdir");
    
    // Create a test file
    {
        std::ofstream f(temp_dir / "test.txt");
        f << "test content";
    }
    {
        std::ofstream f(temp_dir / "subdir" / "nested.txt");
        f << "nested content";
    }
    
    StaticFileOptions options;
    options.root = temp_dir;
    StaticFileServer server(options);
    
    SECTION("Normal paths are allowed") {
        CHECK(server.is_path_allowed(temp_dir / "test.txt"));
        CHECK(server.is_path_allowed(temp_dir / "subdir" / "nested.txt"));
    }
    
    SECTION("Path traversal is blocked") {
        CHECK_FALSE(server.is_path_allowed(temp_dir / ".." / "etc" / "passwd"));
        CHECK_FALSE(server.is_path_allowed(temp_dir / "subdir" / ".." / ".." / "etc"));
    }
    
    // Cleanup
    std::filesystem::remove_all(temp_dir);
}

TEST_CASE("StaticFileOptions extension configuration", "[static_files]") {
    StaticFileOptions options;
    options.root = ".";
    
    SECTION("Default denied extensions list") {
        // Verify default denied extensions are set
        CHECK(options.denied_extensions.size() > 0);
        
        // Check some expected defaults
        bool has_exe = false, has_dll = false, has_sh = false;
        for (const auto& ext : options.denied_extensions) {
            if (ext == ".exe") has_exe = true;
            if (ext == ".dll") has_dll = true;
            if (ext == ".sh") has_sh = true;
        }
        CHECK(has_exe);
        CHECK(has_dll);
        CHECK(has_sh);
    }
    
    SECTION("Custom allowed extensions") {
        options.allowed_extensions = {".html", ".css", ".js"};
        CHECK(options.allowed_extensions.size() == 3);
    }
}
