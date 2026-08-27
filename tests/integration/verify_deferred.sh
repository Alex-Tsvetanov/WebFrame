#!/usr/bin/env bash
#
# Proves a deferred page actually streams.
#
# Not just that the bytes all arrive, which a normal response would also manage, but
# that the shell arrives well before the value does. That gap is the entire point: a
# reader sees the page while the slow query is still running.
#
# Usage:
#   tests/integration/verify_deferred.sh <build-dir> [port] [delay_ms]
set -u

BUILD="${1:-}"
PORT="${2:-14666}"
DELAY="${3:-800}"
HERE="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
SRC="$(cd "$HERE/../.." && pwd)"

if [ -z "$BUILD" ] || [ ! -f "$BUILD/libcoroute.a" ]; then
    echo "usage: $0 <build-dir> [port] [delay_ms]" >&2
    exit 2
fi

# The library may or may not have been built with HTTP/3 and TLS. Nothing here needs
# either, but a static archive still has to resolve every symbol it contains, so the
# link has to match whatever the build actually included.
EXTRA_DEFS=""
EXTRA_LIBS=""
if nm -C "$BUILD/libcoroute.a" 2>/dev/null | grep -q "Http3Endpoint"; then
    QUIC_PREFIX="${QUIC_PREFIX:-$HOME/opt/quic}"
    export PKG_CONFIG_PATH="$QUIC_PREFIX/lib/pkgconfig:${PKG_CONFIG_PATH:-}"
    EXTRA_DEFS="-DCOROUTE_HAS_HTTP3 -DCOROUTE_HAS_TLS"
    EXTRA_LIBS="$(pkg-config --libs libngtcp2_crypto_ossl libngtcp2 libnghttp3) -L$QUIC_PREFIX/lib -lssl -lcrypto -luring"
    # The locally built OpenSSL 3.5 is not on the default search path, and the system
    # 3.0 does not export the symbols this links against.
    export LD_LIBRARY_PATH="$QUIC_PREFIX/lib:${LD_LIBRARY_PATH:-}"
elif nm -C "$BUILD/libcoroute.a" 2>/dev/null | grep -q "TlsContext"; then
    EXTRA_DEFS="-DCOROUTE_HAS_TLS"
    EXTRA_LIBS="-lssl -lcrypto -luring"
fi

echo "== building the deferred page server =="
g++ -std=c++23 -g -O0 -o "$BUILD/deferred_app" "$HERE/deferred_app.cpp" \
    -DCOROUTE_HAS_TEMPLATES -DCOROUTE_HAS_HTTP2 $EXTRA_DEFS \
    -I"$SRC/include" \
    -I"$BUILD/_deps/url_matcher-src/include" \
    -I"$BUILD/_deps/simdjson-src/include" \
    -I"$BUILD/_deps/inja-src/include" \
    -I"$BUILD/_deps/inja-src/third_party/include" \
    -I"$BUILD/_deps/nghttp2-src/lib/includes" \
    -I"$BUILD/_deps/nghttp2-build/lib/includes" \
    "$BUILD/libcoroute.a" \
    "$BUILD/_deps/simdjson-build/libsimdjson.a" \
    "$BUILD/_deps/nghttp2-build/lib/libnghttp2.a" \
    $EXTRA_LIBS -lz -lpthread || exit 1

echo "== starting it on $PORT, slow field takes ${DELAY}ms =="
"$BUILD/deferred_app" "$PORT" "$DELAY" "$HERE/templates" > "$BUILD/deferred_app.log" 2>&1 &
SERVER=$!
trap 'kill "$SERVER" 2>/dev/null' EXIT
sleep 2
if ! kill -0 "$SERVER" 2>/dev/null; then
    echo "server exited early:" >&2
    cat "$BUILD/deferred_app.log" >&2
    exit 1
fi

FAILED=0
check() {
    if grep -qF "$2" "$1"; then
        echo "  ok: $2"
    else
        echo "  MISSING: $2" >&2
        FAILED=1
    fi
}

echo
echo "== the response is streamed, not buffered =="
# -N disables curl's own buffering, so a line is timestamped when it arrives rather
# than when curl decides to flush.
#
# date is called per line rather than using awk's systime(), which has one-second
# resolution and would report every line as simultaneous on a page that completes in
# under a second.
curl -sN "http://127.0.0.1:$PORT/dashboard" 2>/dev/null \
    | while IFS= read -r line; do
          printf '%s %s\n' "$(date +%s%3N)" "$line"
      done > "$BUILD/deferred_timeline.txt"

head -1 "$BUILD/deferred_timeline.txt" | cut -d' ' -f2- | sed 's/^/  first line: /'

shell_at=$(grep -m1 'visitors now' "$BUILD/deferred_timeline.txt" | cut -d' ' -f1)
# The call, not the definition. "__resolve" on its own also matches the runtime's
# api.__resolve = function, which is emitted in the head and so arrives before the
# shell, which made this measurement read as a negative gap.
value_at=$(grep -m1 '__resolve(0,' "$BUILD/deferred_timeline.txt" | cut -d' ' -f1)

if [ -z "$shell_at" ] || [ -z "$value_at" ]; then
    echo "  MISSING: could not find both the shell and the resolve chunk" >&2
    sed 's/^/    /' "$BUILD/deferred_timeline.txt" >&2
    exit 1
fi

gap=$((value_at - shell_at))
echo "  shell to value: ${gap}ms (the slow field was told to take ${DELAY}ms)"

# The claim. If the shell waited for the value there would be no gap, and the whole
# exercise would have bought nothing.
if [ "$gap" -lt $((DELAY / 2)) ]; then
    echo "  FAILED: the shell did not arrive meaningfully before the value" >&2
    FAILED=1
else
    echo "  ok: the shell arrived first, by most of the delay"
fi

echo
echo "== the page carries both client paths =="
cut -d' ' -f2- "$BUILD/deferred_timeline.txt" > "$BUILD/deferred_body.html"
check "$BUILD/deferred_body.html" "api.deferred = function"
check "$BUILD/deferred_body.html" "visitors now: 17"
check "$BUILD/deferred_body.html" 'data-coroute-slot="0"'
check "$BUILD/deferred_body.html" "coroute.deferred(0)"
check "$BUILD/deferred_body.html" "__resolve(0,"
check "$BUILD/deferred_body.html" "the slow part"

if [ "$FAILED" -ne 0 ]; then
    echo
    echo "--- server log ---" >&2
    cat "$BUILD/deferred_app.log" >&2
    exit 1
fi

echo
echo "Deferred rendering confirmed: shell first, value later, both client paths present."
