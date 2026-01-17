# Hello World example
add_subdirectory(hello_world)

# Echo server example
add_subdirectory(echo_server)

# Static file server example
add_subdirectory(static_server)

# Streaming server example
add_subdirectory(streaming_server)

# Range request example
add_subdirectory(range_server)

# WebSocket server example
add_subdirectory(websocket_server)

# HTTP/2 example
if(COROUTE_ENABLE_HTTP2)
    add_subdirectory(http2_server)
endif()

# HTTPS server example (requires TLS support)
if(OpenSSL_FOUND OR TARGET ssl)
    add_subdirectory(https_server)
endif()

# Template server example
add_subdirectory(template_server)

# Benchmark server (no compression)
add_subdirectory(benchmark_server)

# Raw benchmark example
add_subdirectory(raw_benchmark)
