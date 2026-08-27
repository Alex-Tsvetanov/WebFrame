#!/usr/bin/env bash
#
# Drives this server's HTTP/3 endpoint with ngtcp2's reference client.
#
# Why this is a script and not a ctest case
# -----------------------------------------
# It needs a second QUIC implementation to talk to, and that implementation is not
# a build dependency. Protocol code that has never completed a handshake against
# something it did not itself write is not code anyone should trust, so this exists
# even though it cannot run unattended in the ordinary test job.
#
# Prerequisites, in order:
#   scripts/build-quic-deps.sh     builds OpenSSL 3.5, nghttp3, ngtcp2
#   scripts/build-quic-client.sh   builds the reference client and server
#   a CMake build with -DCOROUTE_ENABLE_HTTP3=ON
#
# Usage:
#   tests/integration/verify_http3.sh <build-dir> [prefix] [port]
set -u

BUILD="${1:-}"
PREFIX="${2:-$HOME/opt/quic}"
PORT="${3:-14444}"
HERE="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
SRC="$(cd "$HERE/../.." && pwd)"
CERTS="$PREFIX/testcerts"
EXAMPLES="$PREFIX/src/ngtcp2/build-examples/examples"

if [ -z "$BUILD" ] || [ ! -f "$BUILD/libcoroute.a" ]; then
    echo "usage: $0 <build-dir> [prefix] [port]" >&2
    echo "  <build-dir> must contain libcoroute.a built with COROUTE_ENABLE_HTTP3=ON" >&2
    exit 2
fi
if [ ! -x "$EXAMPLES/osslclient" ]; then
    echo "reference client not found at $EXAMPLES/osslclient" >&2
    echo "run scripts/build-quic-client.sh first" >&2
    exit 2
fi

export PKG_CONFIG_PATH="$PREFIX/lib/pkgconfig:${PKG_CONFIG_PATH:-}"
export LD_LIBRARY_PATH="$PREFIX/lib:${LD_LIBRARY_PATH:-}"

# A self-signed certificate is enough: the client is told not to verify. Generated
# with the system openssl, because the locally built 3.5 is installed with
# "make install_sw" and so has no openssl.cnf, which req needs.
mkdir -p "$CERTS"
if [ ! -s "$CERTS/cert.pem" ]; then
    env -u LD_LIBRARY_PATH openssl req -x509 -newkey rsa:2048 -nodes \
        -keyout "$CERTS/key.pem" -out "$CERTS/cert.pem" -days 30 \
        -subj "/CN=localhost" -addext "subjectAltName=DNS:localhost,IP:127.0.0.1" 2>/dev/null
fi

echo "== building the test server =="
g++ -std=c++23 -g -O0 -o "$BUILD/http3_server" "$HERE/http3_server.cpp" \
    -DCOROUTE_HAS_HTTP3 -DCOROUTE_HAS_TLS \
    -I"$SRC/include" \
    -I"$BUILD/_deps/url_matcher-src/include" \
    -I"$BUILD/_deps/simdjson-src/include" \
    -I"$BUILD/_deps/nghttp2-src/lib/includes" \
    -I"$BUILD/_deps/nghttp2-build/lib/includes" \
    -I"$PREFIX/include" \
    "$BUILD/libcoroute.a" \
    $(pkg-config --libs libngtcp2_crypto_ossl libngtcp2 libnghttp3) \
    "$BUILD/_deps/simdjson-build/libsimdjson.a" \
    "$BUILD/_deps/nghttp2-build/lib/libnghttp2.a" \
    -L"$PREFIX/lib" -lssl -lcrypto -luring -lz -lpthread || exit 1

echo "== starting it on UDP $PORT =="
"$BUILD/http3_server" "$PORT" "$CERTS/key.pem" "$CERTS/cert.pem" > "$BUILD/http3_server.log" 2>&1 &
SERVER=$!
trap 'kill "$SERVER" 2>/dev/null' EXIT
sleep 2
if ! kill -0 "$SERVER" 2>/dev/null; then
    echo "server exited early:" >&2
    cat "$BUILD/http3_server.log" >&2
    exit 1
fi

echo "== reference client against it =="
# Without --exit-on-first-stream-close the client sits there after the response and
# looks exactly like a hang.
timeout 20 "$EXAMPLES/osslclient" --exit-on-first-stream-close \
    127.0.0.1 "$PORT" "https://127.0.0.1:$PORT/" > "$BUILD/http3_client.log" 2>&1
CLIENT_STATUS=$?

# Three things have to be true, and each fails differently: no ALPN means the TLS
# context is wrong, no :status means the request never reached a handler, and no
# body means the response was framed but never sent.
#
# The body is kept under 16 characters on purpose. The client hex-dumps it, and the
# ASCII column of a hex dump is 16 bytes wide, so a longer string is split across
# two lines and no single grep can match it.
FAILED=0
for pattern in "Negotiated ALPN is h3" ":status: 200" "coroute h3 ok"; do
    if grep -qF "$pattern" "$BUILD/http3_client.log"; then
        echo "  ok: $pattern"
    else
        echo "  MISSING: $pattern" >&2
        FAILED=1
    fi
done

if [ "$CLIENT_STATUS" -ne 0 ] || [ "$FAILED" -ne 0 ]; then
    echo
    echo "client exit=$CLIENT_STATUS. full logs:" >&2
    tail -30 "$BUILD/http3_client.log" >&2
    echo "--- server ---" >&2
    cat "$BUILD/http3_server.log" >&2
    exit 1
fi

echo
echo "HTTP/3 end to end: handshake, ALPN, routing and response all confirmed."
