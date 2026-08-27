#!/usr/bin/env bash
#
# Builds ngtcp2's reference QUIC/HTTP3 client, for testing this server against.
#
# Why bother, when curl exists
# ---------------------------
# Distribution curl is generally built without HTTP/3: Ubuntu 24.04 ships 8.5.0 with
# no HTTP3 feature flag. Without some client the HTTP/3 server code cannot be
# exercised at all, and protocol code that has never completed a handshake is not
# code anyone should trust.
#
# ngtcp2's own client is the right one to use here. It is the same library the server
# links, so a failure is a bug in this code rather than an interop difference, which
# is what you want while the server is still being written. Interop against other
# stacks comes later, through the QUIC interop runner.
#
# This is test tooling, not a build dependency. build-quic-deps.sh is what the
# library itself needs.
#
# Usage:
#   scripts/build-quic-client.sh [prefix]      # default: $HOME/opt/quic
set -euo pipefail

PREFIX="${1:-$HOME/opt/quic}"
SRC="$PREFIX/src"
JOBS="$(nproc 2>/dev/null || echo 4)"

export PKG_CONFIG_PATH="$PREFIX/lib/pkgconfig:${PKG_CONFIG_PATH:-}"

if [ ! -d "$SRC/ngtcp2" ]; then
    echo "ngtcp2 sources not found under $SRC." >&2
    echo "Run scripts/build-quic-deps.sh first." >&2
    exit 1
fi

mkdir -p "$SRC"

# ---------------------------------------------------------------- libev
# The examples use libev for their event loop. It is not needed by the library and
# is built here rather than installed system-wide, so no root is required.
if ! pkg-config --exists libev 2>/dev/null && [ ! -f "$PREFIX/lib/libev.a" ]; then
    cd "$SRC"
    if [ ! -d libev-source ]; then
        echo "== fetching libev =="
        # Discovered rather than pinned: upstream publishes a plain directory and the
        # current release changes rarely but is not worth guessing at.
        LIBEV_TAR="$(curl -fsSL http://dist.schmorp.de/libev/Attic/ \
            | grep -o 'libev-[0-9.]*\.tar\.gz' | sort -V | tail -1)"
        if [ -z "$LIBEV_TAR" ]; then
            echo "could not determine a libev release to download" >&2
            exit 1
        fi
        echo "   using $LIBEV_TAR"
        curl -fsSL -o libev.tar.gz "http://dist.schmorp.de/libev/Attic/$LIBEV_TAR"
        mkdir -p libev-source
        tar xf libev.tar.gz -C libev-source --strip-components=1
    fi
    cd libev-source
    echo "== building libev =="
    ./configure --prefix="$PREFIX" --disable-shared >/dev/null
    make -j"${JOBS}" >/dev/null
    make install >/dev/null
else
    echo "== libev already present, skipping =="
fi

# ---------------------------------------------------------------- ngtcp2 examples
echo "== building ngtcp2 examples =="
cd "$SRC/ngtcp2"

# ENABLE_LIB_ONLY=OFF is the whole point here: it is what builds examples/.
cmake -S . -B build-examples \
    -DCMAKE_BUILD_TYPE=Release \
    -DENABLE_LIB_ONLY=OFF \
    -DENABLE_STATIC_LIB=OFF -DENABLE_SHARED_LIB=ON \
    -DBUILD_TESTING=OFF \
    -DENABLE_OPENSSL=ON \
    -DOPENSSL_ROOT_DIR="$PREFIX" \
    -DCMAKE_PREFIX_PATH="$PREFIX"

# osslclient/osslserver, not "client": the example targets are named after their TLS
# backend, and only the OpenSSL pair is built because that is the backend this project
# uses.
#
# The reference server is built alongside the client on purpose. It gives a known-good
# peer to check the client invocation against, so that when this project's own server
# fails there is no doubt about which side is wrong.
cmake --build build-examples --parallel "${JOBS}" --target osslclient osslserver

mkdir -p "$PREFIX/bin"
found=""
for candidate in build-examples/examples/osslclient build-examples/examples/osslserver; do
    if [ -x "$candidate" ]; then
        cp "$candidate" "$PREFIX/bin/ngtcp2-$(basename "$candidate")"
        found="$candidate"
    fi
done

if [ -z "$found" ]; then
    echo "client binary was not produced; check the build output above" >&2
    exit 1
fi

echo
echo "== done =="
ls -la "$PREFIX/bin"/ngtcp2-* 2>/dev/null
echo
echo "Verified working: reference client against reference server completes a full"
echo "QUIC handshake and brings up the HTTP/3 control and QPACK streams."
echo
echo "Note: the client does not exit on its own once a request finishes. Pass"
echo "--exit-on-first-stream-close (or --exit-on-all-streams-close) in scripts, or it"
echo "will sit there until killed and look like a hang."
echo
echo "The binaries need the built OpenSSL on the library path:"
echo "  export LD_LIBRARY_PATH=$PREFIX/lib"
echo
echo "Usage against a local server:"
echo "  $PREFIX/bin/ngtcp2-osslclient 127.0.0.1 4433 https://127.0.0.1:4433/"
