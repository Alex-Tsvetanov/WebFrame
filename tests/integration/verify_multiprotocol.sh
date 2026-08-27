#!/usr/bin/env bash
#
# One App, one port number, three protocols.
#
# Checks that a single server instance answers HTTP/1.1 and HTTP/2 over TCP and
# HTTP/3 over UDP, and that the TCP responses advertise the HTTP/3 endpoint through
# Alt-Svc. Without that header the UDP side is reachable in theory and never used,
# because a browser will not try HTTP/3 unless it is told the endpoint exists.
#
# Prerequisites:
#   scripts/build-quic-deps.sh, scripts/build-quic-client.sh
#   a CMake build with -DCOROUTE_ENABLE_HTTP3=ON
#
# Usage:
#   tests/integration/verify_multiprotocol.sh <build-dir> [prefix] [port]
set -u

BUILD="${1:-}"
PREFIX="${2:-$HOME/opt/quic}"
PORT="${3:-14555}"
# Four workers, so the run exercises the sharded path: several UDP sockets on one
# port through SO_REUSEPORT, and connection-ID steering between them.
THREADS="${4:-4}"
HERE="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
SRC="$(cd "$HERE/../.." && pwd)"
CERTS="$PREFIX/testcerts"
EXAMPLES="$PREFIX/src/ngtcp2/build-examples/examples"

if [ -z "$BUILD" ] || [ ! -f "$BUILD/libcoroute.a" ]; then
    echo "usage: $0 <build-dir> [prefix] [port]" >&2
    exit 2
fi

export PKG_CONFIG_PATH="$PREFIX/lib/pkgconfig:${PKG_CONFIG_PATH:-}"
export LD_LIBRARY_PATH="$PREFIX/lib:${LD_LIBRARY_PATH:-}"

mkdir -p "$CERTS"
if [ ! -s "$CERTS/cert.pem" ]; then
    env -u LD_LIBRARY_PATH openssl req -x509 -newkey rsa:2048 -nodes \
        -keyout "$CERTS/key.pem" -out "$CERTS/cert.pem" -days 30 \
        -subj "/CN=localhost" -addext "subjectAltName=DNS:localhost,IP:127.0.0.1" 2>/dev/null
fi

echo "== building the multi-protocol server =="
g++ -std=c++23 -g -O0 -o "$BUILD/http3_app" "$HERE/http3_app.cpp" \
    -DCOROUTE_HAS_HTTP3 -DCOROUTE_HAS_TLS -DCOROUTE_HAS_HTTP2 \
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

echo "== starting it on $PORT with $THREADS worker(s) =="
"$BUILD/http3_app" "$PORT" "$CERTS/key.pem" "$CERTS/cert.pem" "$THREADS" > "$BUILD/http3_app.log" 2>&1 &
SERVER=$!
trap 'kill "$SERVER" 2>/dev/null' EXIT
sleep 2
if ! kill -0 "$SERVER" 2>/dev/null; then
    echo "server exited early:" >&2
    cat "$BUILD/http3_app.log" >&2
    exit 1
fi

FAILED=0
check() {
    if grep -qiF "$2" "$1"; then
        echo "  ok: $2"
    else
        echo "  MISSING: $2" >&2
        FAILED=1
    fi
}

# The certificate is self-signed, so verification is off. That is the only thing
# switched off here: everything else is a normal client request.
echo
echo "== HTTP/1.1 over TLS =="
curl -sSik --http1.1 "https://127.0.0.1:$PORT/" > "$BUILD/h1.log" 2>&1
check "$BUILD/h1.log" "HTTP/1.1 200"
check "$BUILD/h1.log" "coroute h3 ok"
# The same middleware runs for every protocol, so the header must appear here too.
check "$BUILD/h1.log" "alt-svc: h3="

echo
echo "== HTTP/2 over TLS, same port and same server =="
curl -sSik --http2 "https://127.0.0.1:$PORT/" > "$BUILD/h2.log" 2>&1
check "$BUILD/h2.log" "HTTP/2 200"
check "$BUILD/h2.log" "coroute h3 ok"
check "$BUILD/h2.log" "alt-svc: h3="

echo
echo "== HTTP/3 over QUIC, same port number over UDP =="
timeout 20 "$EXAMPLES/osslclient" --exit-on-first-stream-close \
    127.0.0.1 "$PORT" "https://127.0.0.1:$PORT/" > "$BUILD/h3.log" 2>&1
check "$BUILD/h3.log" "Negotiated ALPN is h3"
check "$BUILD/h3.log" ":status: 200"
check "$BUILD/h3.log" "coroute h3 ok"

echo
echo "== QUIC packet steering =="
# Reported by the server about itself. forwarded_in over received says how much work
# kernel-side steering could have saved, which is the question eBPF would answer.
curl -sSk --http1.1 "https://127.0.0.1:$PORT/quic-stats" | sed 's/^/  /'

echo
echo "== descriptor count =="
# The point of the exercise, stated precisely so the number cannot flatter itself.
#
# The claim is one listening descriptor per transport per worker, independent of how
# many protocols are served: N TCP and N UDP for N workers, whether that is HTTP/1.1
# alone or all three at once. It is not one descriptor absolutely. The per-worker
# sockets exist so the kernel can spread connections across threads, which is a
# separate axis from the protocol mix and would be needed for a single-protocol
# server too.
#
# So the figure to compare against nginx is TCP-listeners-per-worker, where a
# configuration serving cleartext and TLS needs two because ssl and non-ssl cannot
# share a listen directive.
tcp_count=$(ss -lntp 2>/dev/null | grep -c "pid=$SERVER,")
udp_count=$(ss -lnup 2>/dev/null | grep -c "pid=$SERVER,")
echo "  workers:        $THREADS"
echo "  TCP listeners:  $tcp_count  (expected: one per worker)"
echo "  UDP sockets:    $udp_count  (expected: one per worker)"
echo "  protocols served on them: HTTP/1.1, HTTP/2, HTTP/3"
ss -lntup 2>/dev/null | grep "pid=$SERVER," | awk '{print "  " $1 " " $5}'

if [ "$FAILED" -ne 0 ]; then
    echo
    echo "--- server log ---" >&2
    cat "$BUILD/http3_app.log" >&2
    exit 1
fi

echo
echo "One App, one port number, three protocols. All confirmed."
