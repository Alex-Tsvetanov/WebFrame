#!/usr/bin/env bash
#
# Builds the HTTP/3 dependency stack into a single prefix: OpenSSL 3.5, nghttp3 and
# ngtcp2 (with its OpenSSL crypto backend).
#
# Why a script rather than FetchContent
# ------------------------------------
# nghttp2, nghttp3 and ngtcp2 each call add_custom_target(check) unconditionally, and
# CMake targets are global, so a build that pulls in more than one of them fails with
# "another target with the same name already exists". No option disables it; ngtcp2
# and nghttp3 also collide on a "main" test executable. Building them separately and
# discovering them with pkg-config is how curl consumes the same libraries.
#
# Why a private OpenSSL
# ---------------------
# ngtcp2's libngtcp2_crypto_ossl needs the QUIC TLS API added in OpenSSL 3.5.
# Distributions lag: Ubuntu 24.04 ships 3.0.13 with no newer candidate. Building 3.5
# here keeps one TLS library across TCP and QUIC instead of adding a second stack for
# the QUIC half.
#
# Usage:
#   scripts/build-quic-deps.sh [prefix]
#
# Then configure with:
#   cmake -B build -DCOROUTE_ENABLE_HTTP3=ON \
#         -DOPENSSL_ROOT_DIR=<prefix> \
#         -DCMAKE_PREFIX_PATH=<prefix>
#
# The versions below are pinned so a rebuild reproduces the same stack. Record them
# alongside any measurement taken with this build.
set -euo pipefail

OPENSSL_VERSION="3.5.8"
NGHTTP3_VERSION="v1.18.0"
NGTCP2_VERSION="v1.25.0"

PREFIX="${1:-$HOME/opt/quic}"
SRC="$PREFIX/src"
JOBS="$(nproc 2>/dev/null || echo 4)"

mkdir -p "$SRC"
export PKG_CONFIG_PATH="$PREFIX/lib/pkgconfig:${PKG_CONFIG_PATH:-}"

echo "== prefix: $PREFIX (${JOBS} jobs) =="

# ---------------------------------------------------------------- OpenSSL 3.5
if [ ! -f "$PREFIX/lib/libssl.so" ] && [ ! -f "$PREFIX/lib64/libssl.so" ]; then
    cd "$SRC"
    if [ ! -d "openssl-openssl-${OPENSSL_VERSION}" ]; then
        echo "== fetching OpenSSL ${OPENSSL_VERSION} =="
        curl -fsSL -o "openssl-${OPENSSL_VERSION}.tar.gz" \
            "https://github.com/openssl/openssl/archive/refs/tags/openssl-${OPENSSL_VERSION}.tar.gz"
        tar xf "openssl-${OPENSSL_VERSION}.tar.gz"
    fi
    cd "openssl-openssl-${OPENSSL_VERSION}"
    [ -f Makefile ] || ./Configure --prefix="$PREFIX" --openssldir="$PREFIX/ssl" \
        --libdir=lib shared enable-quic no-docs
    echo "== building OpenSSL =="
    make -j"${JOBS}"
    make install_sw
else
    echo "== OpenSSL already present, skipping =="
fi

# ---------------------------------------------------------------- nghttp3
if ! pkg-config --exists libnghttp3 2>/dev/null; then
    cd "$SRC"
    [ -d nghttp3 ] || git clone --depth 1 --branch "$NGHTTP3_VERSION" \
        https://github.com/ngtcp2/nghttp3.git
    cd nghttp3
    git submodule update --init --depth 1
    echo "== building nghttp3 ${NGHTTP3_VERSION} =="
    cmake -S . -B build -DCMAKE_INSTALL_PREFIX="$PREFIX" \
        -DCMAKE_BUILD_TYPE=Release \
        -DENABLE_LIB_ONLY=ON -DENABLE_STATIC_LIB=ON -DENABLE_SHARED_LIB=OFF \
        -DBUILD_TESTING=OFF
    cmake --build build --parallel "${JOBS}"
    cmake --install build
else
    echo "== nghttp3 already present, skipping =="
fi

# ---------------------------------------------------------------- ngtcp2
if ! pkg-config --exists libngtcp2_crypto_ossl 2>/dev/null; then
    cd "$SRC"
    [ -d ngtcp2 ] || git clone --depth 1 --branch "$NGTCP2_VERSION" \
        https://github.com/ngtcp2/ngtcp2.git
    cd ngtcp2
    git submodule update --init --depth 1
    echo "== building ngtcp2 ${NGTCP2_VERSION} =="
    # ENABLE_OPENSSL selects libngtcp2_crypto_ossl. ngtcp2 probes the OpenSSL it finds
    # for QUIC support and fails configuration if it is missing, which is the failure
    # we want: silently falling back to a non-QUIC TLS stack would be far worse.
    cmake -S . -B build -DCMAKE_INSTALL_PREFIX="$PREFIX" \
        -DCMAKE_BUILD_TYPE=Release \
        -DENABLE_LIB_ONLY=ON -DENABLE_STATIC_LIB=ON -DENABLE_SHARED_LIB=OFF \
        -DBUILD_TESTING=OFF \
        -DENABLE_OPENSSL=ON \
        -DOPENSSL_ROOT_DIR="$PREFIX"
    cmake --build build --parallel "${JOBS}"
    cmake --install build
else
    echo "== ngtcp2 already present, skipping =="
fi

echo
echo "== done =="
for m in openssl libnghttp3 libngtcp2 libngtcp2_crypto_ossl; do
    printf '  %-24s %s\n' "$m" "$(pkg-config --modversion "$m" 2>/dev/null || echo MISSING)"
done
echo
echo "Configure coroute with:"
echo "  cmake -B build -DCOROUTE_ENABLE_HTTP3=ON -DOPENSSL_ROOT_DIR=$PREFIX -DCMAKE_PREFIX_PATH=$PREFIX"
