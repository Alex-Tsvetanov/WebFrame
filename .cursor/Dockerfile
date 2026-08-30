FROM ubuntu:24.04

ENV DEBIAN_FRONTEND=noninteractive

# System toolchain and stable dependencies for the Coroute (WebFrame) C++20 build.
# Repository dependencies (nghttp2, simdjson, inja, Catch2, RegexMatcher) are
# fetched at configure time via CMake FetchContent in the install phase.
RUN apt-get update \
    && apt-get install -y --no-install-recommends \
        build-essential \
        g++ \
        clang \
        cmake \
        ninja-build \
        pkg-config \
        git \
        ca-certificates \
        curl \
        libssl-dev \
        zlib1g-dev \
        liburing-dev \
    && rm -rf /var/lib/apt/lists/*

USER ubuntu
