#!/usr/bin/env bash
#
# Force a QUIC 4-tuple change mid-connection and assert userspace CID forwarding fires.
#
# Why this exists
# ---------------
# SO_REUSEPORT spreads UDP datagrams by hashing the 4-tuple. QUIC connections survive
# address changes, so after a migration the hash can land a packet on a worker that
# does not own the connection. The server encodes the owning worker in the connection
# ID and forwards in userspace. Until something in the suite actually migrates, the
# forwarded_out / forwarded_in counters stay at zero and that path is untested.
#
# How migration is forced
# -----------------------
# A client network namespace reaches the server over a veth. After the handshake, nft
# SNAT remaps the client's UDP source port so later datagrams present a new 4-tuple
# to the server. That is NAT rebinding from the server's point of view: the reference
# client keeps its socket; only the on-wire source port changes.
#
# The ngtcp2 client also has --change-local-addr/--nat-rebinding (binds a second UDP
# socket). SNAT is used here because it matches the measurement plan and works without
# client cooperation; the client still has to be a real second implementation.
#
# Worker count
# ------------
# THREADS defaults to 4. More than one worker is required: with a single worker there
# is no other socket for SO_REUSEPORT to hash to, so forwarding cannot fire. Four is
# enough for a hash miss to be likely across a handful of SNAT port retries.
#
# Linux only. Requires netns + nft + CAP_NET_ADMIN (or root / unshare -Urm). Skips
# (exit 77) when HTTP/3, the reference client, or the netns/nft harness is missing.
#
# Prerequisites:
#   scripts/build-quic-deps.sh
#   scripts/build-quic-client.sh   (needs a C++23 toolchain with <print>, e.g. g++-14)
#   cmake -B <build> -DCOROUTE_ENABLE_HTTP3=ON ...
#
# Usage:
#   tests/integration/verify_http3_migration.sh <build-dir> [prefix] [port] [threads]
#
# ctest registers this as http3_path_migration. Exit 77 = skipped.
set -u

BUILD="${1:-}"
PREFIX="${2:-$HOME/opt/quic}"
PORT="${3:-14666}"
# Four workers so SO_REUSEPORT has somewhere else to send a remapped 4-tuple.
THREADS="${4:-4}"
HERE="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
SRC="$(cd "$HERE/../.." && pwd)"
CERTS="$PREFIX/testcerts"
EXAMPLES="$PREFIX/src/ngtcp2/build-examples/examples"
CLIENT=""
RESULTS_DIR="$SRC/tests/results"
LOG="$RESULTS_DIR/http3_path_migration.log"
SKIP=77

mkdir -p "$RESULTS_DIR"
: > "$LOG"
exec > >(tee -a "$LOG") 2>&1

skip() {
    echo "SKIP: $*"
    exit "$SKIP"
}

fail() {
    echo "FAIL: $*" >&2
    exit 1
}

echo "== HTTP/3 path migration / CID forwarding =="
echo "build=$BUILD prefix=$PREFIX port=$PORT threads=$THREADS"
echo "host=$(uname -s) $(uname -r)"
date -u +"utc=%Y-%m-%dT%H:%M:%SZ"

# ---- gate: HTTP/3 build -------------------------------------------------------
if [ -z "$BUILD" ] || [ ! -f "$BUILD/libcoroute.a" ]; then
    skip "no libcoroute.a in build dir (need -DCOROUTE_ENABLE_HTTP3=ON build)"
fi
if ! nm -C "$BUILD/libcoroute.a" 2>/dev/null | grep -q 'Http3Endpoint'; then
    skip "libcoroute.a has no Http3Endpoint symbols (HTTP/3 not enabled in this build)"
fi

# ---- gate: reference client ---------------------------------------------------
for candidate in \
    "$EXAMPLES/osslclient" \
    "$PREFIX/bin/ngtcp2-osslclient" \
    "$PREFIX/bin/osslclient"
do
    if [ -x "$candidate" ]; then
        CLIENT="$candidate"
        break
    fi
done
if [ -z "$CLIENT" ]; then
    skip "ngtcp2 osslclient not found (run scripts/build-quic-client.sh)"
fi
echo "client=$CLIENT"

# ---- gate: Linux netns + nft --------------------------------------------------
if [ "$(uname -s)" != "Linux" ]; then
    skip "Linux-only: netns/nft path migration harness"
fi
command -v unshare >/dev/null || skip "unshare not available"
command -v ip >/dev/null || skip "ip (iproute2) not available"
command -v nft >/dev/null || skip "nft (nftables) not available"

# Need a netns we can configure. Prefer root; otherwise a user+mount+net namespace.
NS_PREFIX="coroute-h3mig-$$"
SERVER_NS="${NS_PREFIX}-srv"
CLIENT_NS="${NS_PREFIX}-cli"
RUN_IN_NS=()
CLEANUP_NS=()

if [ "$(id -u)" -eq 0 ] || sudo -n true 2>/dev/null; then
    as_root() { if [ "$(id -u)" -eq 0 ]; then "$@"; else sudo -n "$@"; fi; }
    as_root ip netns add "$SERVER_NS" || skip "cannot create server netns"
    as_root ip netns add "$CLIENT_NS" || { as_root ip netns del "$SERVER_NS" 2>/dev/null; skip "cannot create client netns"; }
    RUN_IN_NS=(as_root ip netns exec)
    CLEANUP_NS=("$SERVER_NS" "$CLIENT_NS")
else
    # Unprivileged path: one user namespace owning both netns via a helper script is
    # awkward; require root/sudo for the harness and skip cleanly otherwise.
    skip "netns+nft harness needs root or passwordless sudo"
fi

cleanup() {
    local srv="${SERVER_PID:-}"
    if [ -n "$srv" ]; then
        kill "$srv" 2>/dev/null || true
        wait "$srv" 2>/dev/null || true
    fi
    if [ -n "${CLIENT_PID:-}" ]; then
        kill "$CLIENT_PID" 2>/dev/null || true
        wait "$CLIENT_PID" 2>/dev/null || true
    fi
    for ns in "${CLEANUP_NS[@]:-}"; do
        as_root ip netns del "$ns" 2>/dev/null || true
    done
}
trap cleanup EXIT

# ---- topology: veth between server and client namespaces ----------------------
# Server: 10.67.0.1/24   Client: 10.67.0.2/24
VETH_S=veth-s0
VETH_C=veth-c0
SERVER_IP=10.67.0.1
CLIENT_IP=10.67.0.2

as_root ip link add "$VETH_S" type veth peer name "$VETH_C"
as_root ip link set "$VETH_S" netns "$SERVER_NS"
as_root ip link set "$VETH_C" netns "$CLIENT_NS"

"${RUN_IN_NS[@]}" "$SERVER_NS" ip addr add "$SERVER_IP/24" dev "$VETH_S"
"${RUN_IN_NS[@]}" "$SERVER_NS" ip link set "$VETH_S" up
"${RUN_IN_NS[@]}" "$SERVER_NS" ip link set lo up

"${RUN_IN_NS[@]}" "$CLIENT_NS" ip addr add "$CLIENT_IP/24" dev "$VETH_C"
"${RUN_IN_NS[@]}" "$CLIENT_NS" ip link set "$VETH_C" up
"${RUN_IN_NS[@]}" "$CLIENT_NS" ip link set lo up

# nft in the client namespace: table ready, rule applied after the handshake.
"${RUN_IN_NS[@]}" "$CLIENT_NS" nft delete table ip coroute_mig 2>/dev/null || true
"${RUN_IN_NS[@]}" "$CLIENT_NS" nft -f - <<EOF
table ip coroute_mig {
    chain postrouting {
        type nat hook postrouting priority 100;
    }
}
EOF

export PKG_CONFIG_PATH="$PREFIX/lib/pkgconfig:${PKG_CONFIG_PATH:-}"
export LD_LIBRARY_PATH="$PREFIX/lib:${LD_LIBRARY_PATH:-}"

mkdir -p "$CERTS"
if [ ! -s "$CERTS/cert.pem" ]; then
    env -u LD_LIBRARY_PATH openssl req -x509 -newkey rsa:2048 -nodes \
        -keyout "$CERTS/key.pem" -out "$CERTS/cert.pem" -days 30 \
        -subj "/CN=localhost" -addext "subjectAltName=DNS:localhost,IP:127.0.0.1,IP:$SERVER_IP" 2>/dev/null
fi

echo "== building http3_app ($THREADS workers) =="
# Prefer g++-14 when present: the reference client needs it, keep the toolchain consistent.
CXX_BIN=g++
command -v g++-14 >/dev/null && CXX_BIN=g++-14
"$CXX_BIN" -std=c++23 -g -O0 -o "$BUILD/http3_app_mig" "$HERE/http3_app.cpp" \
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
    -L"$PREFIX/lib" -lssl -lcrypto -luring -lz -lpthread || fail "failed to build http3_app"

echo "== starting server in netns $SERVER_NS =="
"${RUN_IN_NS[@]}" "$SERVER_NS" env LD_LIBRARY_PATH="$LD_LIBRARY_PATH" \
    "$BUILD/http3_app_mig" "$PORT" "$CERTS/key.pem" "$CERTS/cert.pem" "$THREADS" \
    > "$BUILD/http3_mig_server.log" 2>&1 &
SERVER_PID=$!
sleep 2
if ! kill -0 "$SERVER_PID" 2>/dev/null; then
    cat "$BUILD/http3_mig_server.log" >&2
    fail "server exited early"
fi
# Confirm multi-worker UDP sockets (one per worker under SO_REUSEPORT).
udp_count=$("${RUN_IN_NS[@]}" "$SERVER_NS" ss -lnup 2>/dev/null | grep -c ":$PORT" || true)
echo "server_pid=$SERVER_PID udp_sockets_on_port=$udp_count (expect $THREADS)"
if [ "${udp_count:-0}" -lt 2 ]; then
    cat "$BUILD/http3_mig_server.log" >&2
    fail "need >=2 UDP sockets for a hash miss; got ${udp_count:-0}. Is io_uring worker affinity available?"
fi

read_stats() {
    # Prefer TCP /quic-stats so the counter read does not itself migrate the QUIC path.
    "${RUN_IN_NS[@]}" "$CLIENT_NS" env LD_LIBRARY_PATH="$LD_LIBRARY_PATH" \
        curl -sSk --http1.1 --connect-timeout 3 "https://$SERVER_IP:$PORT/quic-stats" 2>/dev/null \
        || true
}

stat_field() {
    # stdin: stats body; $1: field name
    awk -v k="$1" '$1==k {print $2; found=1} END{if(!found) print 0}'
}

echo "== baseline quic-stats (before migration) =="
BASE_STATS="$(read_stats)"
echo "$BASE_STATS" | sed 's/^/  /'
BASE_OUT="$(printf '%s\n' "$BASE_STATS" | stat_field forwarded_out)"
BASE_IN="$(printf '%s\n' "$BASE_STATS" | stat_field forwarded_in)"
echo "baseline forwarded_out=$BASE_OUT forwarded_in=$BASE_IN"

# ---- migrate: handshake, SNAT, then first stream on the remapped 4-tuple ------
#
# --delay-stream waits after the handshake before opening the HTTP/3 request.
# During that window nft remaps the client's UDP source port. The request packets
# then arrive with a new 4-tuple; SO_REUSEPORT may deliver them to a non-owner
# worker, which must forward by connection ID.
apply_snat() {
    local new_port="$1"
    "${RUN_IN_NS[@]}" "$CLIENT_NS" nft flush chain ip coroute_mig postrouting
    "${RUN_IN_NS[@]}" "$CLIENT_NS" nft add rule ip coroute_mig postrouting \
        ip daddr "$SERVER_IP" udp dport "$PORT" \
        snat to "$CLIENT_IP:$new_port"
    echo "  nft SNAT udp sport -> $new_port"
}

# Try several SNAT ports: the kernel hash is not under our control, so one remapping
# can still land on the owning worker. Each attempt is a fresh connection.
FORWARDED=0
CLIENT_OK=0
ATTEMPT=0
MAX_ATTEMPTS=12
SNAT_PORT=40100

while [ "$ATTEMPT" -lt "$MAX_ATTEMPTS" ]; do
    ATTEMPT=$((ATTEMPT + 1))
    SNAT_PORT=$((SNAT_PORT + 17))
    echo
    echo "== attempt $ATTEMPT/$MAX_ATTEMPTS (SNAT sport $SNAT_PORT) =="

    BEFORE="$(read_stats)"
    BEFORE_OUT="$(printf '%s\n' "$BEFORE" | stat_field forwarded_out)"
    BEFORE_IN="$(printf '%s\n' "$BEFORE" | stat_field forwarded_in)"

    # Clear prior NAT mappings so the new sport is used cleanly.
    "${RUN_IN_NS[@]}" "$CLIENT_NS" nft flush chain ip coroute_mig postrouting

    CLIENT_LOG="$BUILD/http3_mig_client_${ATTEMPT}.log"
    # delay-stream=2s: handshake first; SNAT at ~0.8s; request opens on remapped path.
    "${RUN_IN_NS[@]}" "$CLIENT_NS" env LD_LIBRARY_PATH="$LD_LIBRARY_PATH" \
        timeout 25 "$CLIENT" \
        --delay-stream=2s \
        --exit-on-first-stream-close \
        "$SERVER_IP" "$PORT" "https://$SERVER_IP:$PORT/" \
        > "$CLIENT_LOG" 2>&1 &
    CLIENT_PID=$!

    # Wait for handshake to progress, then remap.
    sleep 0.8
    apply_snat "$SNAT_PORT"

    wait "$CLIENT_PID" || true
    CLIENT_STATUS=$?
    CLIENT_PID=

    # The body is "coroute h3 ok\n" (14 bytes). When the client hex-dumps a STREAM
    # that also carries the HEADERS frame, the ASCII column wraps at 16 and splits
    # "coroute"; match a contiguous fragment instead. Clean stream close is the
    # survival signal; [:status: 200] is printed on some client builds and not others.
    if grep -qF "Negotiated ALPN is h3" "$CLIENT_LOG" \
        && grep -qE 'oute h3 ok|coroute h3 ok|\[:status: 200\]' "$CLIENT_LOG" \
        && grep -qE 'HTTP stream 0x0 closed with error codes \(RX:\(no error\)' "$CLIENT_LOG"; then
        echo "  client: request completed on remapped path"
        CLIENT_OK=1
    else
        echo "  client: incomplete (exit=$CLIENT_STATUS); checking fragments:"
        grep -n 'ALPN\|status\|h3 ok\|stream 0x0 closed' "$CLIENT_LOG" | tail -20 | sed 's/^/    /' || true
        tail -10 "$CLIENT_LOG" | sed 's/^/    /'
    fi

    AFTER="$(read_stats)"
    echo "$AFTER" | sed 's/^/  /'
    AFTER_OUT="$(printf '%s\n' "$AFTER" | stat_field forwarded_out)"
    AFTER_IN="$(printf '%s\n' "$AFTER" | stat_field forwarded_in)"

    DELTA_OUT=$((AFTER_OUT - BEFORE_OUT))
    DELTA_IN=$((AFTER_IN - BEFORE_IN))
    echo "  delta forwarded_out=$DELTA_OUT forwarded_in=$DELTA_IN"

    if [ "$DELTA_OUT" -gt 0 ] || [ "$DELTA_IN" -gt 0 ]; then
        FORWARDED=1
        echo "  userspace forwarding fired"
        break
    fi
done

echo
echo "== final quic-stats =="
FINAL="$(read_stats)"
echo "$FINAL" | sed 's/^/  /'
FINAL_OUT="$(printf '%s\n' "$FINAL" | stat_field forwarded_out)"
FINAL_IN="$(printf '%s\n' "$FINAL" | stat_field forwarded_in)"
FINAL_RX="$(printf '%s\n' "$FINAL" | stat_field received)"
FINAL_ACC="$(printf '%s\n' "$FINAL" | stat_field accepted)"

echo
echo "workers=$THREADS"
echo "attempts=$ATTEMPT"
echo "client_ok=$CLIENT_OK"
echo "forwarded_out=$FINAL_OUT"
echo "forwarded_in=$FINAL_IN"
echo "received=$FINAL_RX"
echo "accepted=$FINAL_ACC"
echo "log=$LOG"

if [ "$CLIENT_OK" -ne 1 ]; then
    echo "--- server log ---" >&2
    cat "$BUILD/http3_mig_server.log" >&2
    fail "connection did not survive migration (no successful remapped request)"
fi
if [ "$FORWARDED" -ne 1 ]; then
    echo "--- server log ---" >&2
    cat "$BUILD/http3_mig_server.log" >&2
    fail "forwarded_out/forwarded_in never incremented after $ATTEMPT SNAT remaps (forwarding did not fire)"
fi

echo
echo "HTTP/3 path migration: connection survived and userspace CID forwarding fired."
exit 0
