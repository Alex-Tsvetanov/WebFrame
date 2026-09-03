#!/usr/bin/env bash
#
# Measurement B of the X2 design: the conditional forwarded share, and whether the model holds.
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
PORT="${3:-14777}"
# Four workers so SO_REUSEPORT has somewhere else to send a remapped 4-tuple.
THREADS="${4:-4}"
HERE="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
SRC="$(cd "$HERE/../.." && pwd)"
CERTS="$PREFIX/testcerts"
EXAMPLES="$PREFIX/src/ngtcp2/build-examples/examples"
CLIENT=""
SKIP=77

# The transcript goes to the build directory, not to $SRC/tests/results.
#
# It used to go in-tree, which is wrong twice over. Every run overwrote the file
# unconditionally, so a run that merely *skipped* -- wrong OS, no client, no
# sudo -- replaced a recorded measurement with five lines saying it did nothing.
# And the header line prints the build and prefix paths of the machine that
# produced it, so an in-tree default writes a local username into a public
# repository on every invocation.
#
# Promoting a run to the record is now a deliberate copy, not a side effect.
[ -n "$BUILD" ] || {
    echo "usage: ${0##*/} <build-dir> [prefix] [port] [threads]" >&2
    exit 1
}
LOG_DIR="${LOG_DIR:-$BUILD}"
LOG="$LOG_DIR/x2_share.log"

mkdir -p "$LOG_DIR"
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

echo "== X2 measurement B: conditional forwarded share =="
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
    # $SERVER_PID is the `ip netns exec` wrapper, not the server: sudo forks, so the
    # process holding the listening sockets is a grandchild and survives the wrapper's
    # death. Every run used to strand a four-worker server, and seven were found alive
    # after one afternoon. `ip netns pids` names exactly the processes in this test's own
    # namespace, so this kills those and nothing else -- pkill on the binary name would
    # reach a concurrent run's server too.
    #
    # TERM first, because the server now installs a handler and shuts down cleanly; KILL
    # after a bounded wait, because a harness that can be left waiting is a harness that
    # hangs. Done before the namespace is deleted, while its pids can still be listed.
    for ns in "${CLEANUP_NS[@]:-}"; do
        local pids
        pids="$(as_root ip netns pids "$ns" 2>/dev/null || true)"
        [ -n "$pids" ] || continue
        # shellcheck disable=SC2086
        as_root kill -TERM $pids 2>/dev/null || true
        for _ in $(seq 1 50); do
            pids="$(as_root ip netns pids "$ns" 2>/dev/null || true)"
            [ -n "$pids" ] || break
            sleep 0.1
        done
        # shellcheck disable=SC2086
        [ -n "$pids" ] && as_root kill -KILL $pids 2>/dev/null || true
    done
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
# Generic receive offload coalesces datagrams before an AF_PACKET tap can see them, so the
# capture reports one large frame where the server counted several datagrams. Measured: 70
# inbound frames over 1400 bytes across ten connections, twenty of them exactly 2400, which
# is two 1200-byte QUIC datagrams merged -- and every connection's wire count was short of
# its counter by about five. A capture that undercounts traffic after a migration would
# understate f_after and manufacture a disagreement with the model it is meant to test.
# ethtool is not installed and installing it is a system change; this does the same job and
# is confined to this test's own namespace.
"${RUN_IN_NS[@]}" "$SERVER_NS" ip link set "$VETH_S" gro_max_size 1500 2>/dev/null || true
"${RUN_IN_NS[@]}" "$SERVER_NS" ip link set "$VETH_S" gso_max_size 1500 2>/dev/null || true
"${RUN_IN_NS[@]}" "$SERVER_NS" python3 "$HERE/nooffload.py" "$VETH_S" || true
"${RUN_IN_NS[@]}" "$SERVER_NS" ip link set lo up

"${RUN_IN_NS[@]}" "$CLIENT_NS" ip addr add "$CLIENT_IP/24" dev "$VETH_C"
"${RUN_IN_NS[@]}" "$CLIENT_NS" ip link set "$VETH_C" up
"${RUN_IN_NS[@]}" "$CLIENT_NS" ip link set "$VETH_C" gso_max_size 1500 2>/dev/null || true
"${RUN_IN_NS[@]}" "$CLIENT_NS" ip link set "$VETH_C" gro_max_size 1500 2>/dev/null || true
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
    COROUTE_BULK_BYTES="${BULK_BYTES:-65536}" \
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
# ---------------------------------------------------------------------------------
# What this measures, and why it is shaped this way
# ---------------------------------------------------------------------------------
#
# The model to be checked is  share = f_after * (N-1)/N , where share is
# forwarded_in/received for a connection that migrates exactly once and f_after is the
# share of that connection's packets sent after the move.
#
# f_after MUST NOT come from the endpoint counters. When a migrated connection lands on a
# non-owning worker every subsequent packet is forwarded, so forwarded_in/received IS
# approximately f_after, and a check that takes both halves from there confirms the model
# by construction and can never be the finding it is supposed to be able to be. So f_after
# is measured on the wire, by a packet capture that is outside the server entirely.
#
# The migration point is observed, not set. A connection's whole request sequence completes
# in a few milliseconds, while the client's --change-local-addr takes a duration, so asking
# for "fifty per cent through the requests" is not something this apparatus can hit: a first
# pilot asked for 1s on a connection that finished in 3.8ms and migrated 260 connection
# lifetimes too late. Instead a spread of short durations is requested, scheduling jitter
# scatters where each one lands, and the wire says where it actually landed. That yields
# many connections at many values of f_after and tests the model as a relation across the
# range rather than at three cells that were never hit exactly.
#
# Connections are long — REQUESTS requests each — because with twenty the forwarded counts
# are small integers and the share is quantised more coarsely than the effect it resolves.
#
# Nothing here is timed. These are counts.

REQUESTS="${REQUESTS:-60}"   # under the server's initial MAX_STREAMS of 100
CONNECTIONS="${CONNECTIONS:-25}"
OUT="${OUT:-$BUILD/x2_share_w${THREADS}.jsonl}"
WIRE="${WIRE:-/tmp/x2_wire_w${THREADS}.jsonl}"

URIS=()
for i in $(seq 1 "$REQUESTS"); do # No query string: "/bulk?i=1" does not match the route "/bulk", and every one of
    # those requests returned 404 with a nine-byte body while its stream still closed
    # cleanly -- so a clean close is not evidence that anything was served. The status is
    # checked below for the same reason.
    URIS+=("https://$SERVER_IP:$PORT${URI_PATH:-/bulk}"); done

: > "$OUT"
as_root rm -f "$WIRE" 2>/dev/null || true

# One capture for the whole cell; connections run one at a time and each uses its own
# source ports, so the record segments cleanly by the boundaries recorded per connection.
"${RUN_IN_NS[@]}" "$SERVER_NS" python3 "$HERE/qsniff.py" "$VETH_S" "$PORT" \
    $((CONNECTIONS * 20 + 120)) "$WIRE" &
rm -f "$WIRE.stop"
sleep 1

echo "workers=$THREADS requests=$REQUESTS bulk_bytes=${BULK_BYTES:-65536} connections=$CONNECTIONS"
echo "wire=$WIRE out=$OUT"

conn=0
while [ "$conn" -lt "$CONNECTIONS" ]; do
    conn=$((conn + 1))
    # A spread of short delays. The values are requested, not achieved; the wire says what
    # happened. Cycling rather than randomising keeps the run reproducible.
    # Spread across the connection's actual life, which is about 100 ms for 60 requests
    # of 64 KiB, so f_after ranges from nearly all of the traffic to nearly none of it.
    # The observed move lands 5-10 ms later than requested, which is why it is read from
    # the wire rather than assumed: these are requests, not settings.
    if [ -n "${DELAYS:-}" ]; then
        # shellcheck disable=SC2086
        set -- $DELAYS
        eval "delay=\${$(( (conn - 1) % $# + 1 ))}"
    else
        case $((conn % 7)) in
            0) delay=5ms ;;  1) delay=15ms ;; 2) delay=30ms ;; 3) delay=45ms ;;
            4) delay=60ms ;; 5) delay=75ms ;; 6) delay=90ms ;;
        esac
    fi

    before="$(read_stats)"
    b_rx="$(printf '%s\n' "$before" | stat_field received)"
    b_in="$(printf '%s\n' "$before" | stat_field forwarded_in)"
    b_out="$(printf '%s\n' "$before" | stat_field forwarded_out)"
    b_acc="$(printf '%s\n' "$before" | stat_field accepted)"
    b_hns="$(printf '%s\n' "$before" | stat_field forward_hop_ns)"
    b_hn="$(printf '%s\n' "$before" | stat_field forward_hop_count)"

    t_start="$(python3 -c 'import time; print(repr(time.time()))')"
    "${RUN_IN_NS[@]}" "$CLIENT_NS" env LD_LIBRARY_PATH="$LD_LIBRARY_PATH" \
        timeout 30 "$CLIENT" --delay-stream="${DELAY_STREAM:-0s}" --change-local-addr="$delay" \
        --exit-on-all-streams-close \
        "$SERVER_IP" "$PORT" "${URIS[@]}" > "$BUILD/x2_client_${THREADS}_${conn}.log" 2>&1 || true
    # A connection's last datagrams arrive after its client exits. Read the counters
    # before they land and they are attributed to the next connection, whose capture
    # window has not opened yet -- which showed up as every connection's wire count being
    # short of its counter by about five, constantly, in both directions of the fix.
    sleep 0.5
    after="$(read_stats)"
    # After the counters are read, not when the client exited: a connection's last
    # datagrams -- its CONNECTION_CLOSE and trailing acknowledgements -- arrive in
    # between, and the server counts them. A window closed at the client's exit was
    # short by about five datagrams per connection, every time.
    t_end="$(python3 -c 'import time; print(repr(time.time()))')"
    a_rx="$(printf '%s\n' "$after" | stat_field received)"
    a_in="$(printf '%s\n' "$after" | stat_field forwarded_in)"
    a_out="$(printf '%s\n' "$after" | stat_field forwarded_out)"
    a_acc="$(printf '%s\n' "$after" | stat_field accepted)"
    a_hns="$(printf '%s\n' "$after" | stat_field forward_hop_ns)"
    a_hn="$(printf '%s\n' "$after" | stat_field forward_hop_count)"

    closed=$(grep -c 'closed with error codes (RX:(no error)' "$BUILD/x2_client_${THREADS}_${conn}.log" || true)
    ok200=$(grep -c ':status: 200' "$BUILD/x2_client_${THREADS}_${conn}.log" || true)
    if [ "$ok200" -ne "$REQUESTS" ]; then
        echo "  conn $conn: only $ok200/$REQUESTS responses were 200; refusing to record this cell" >&2
        grep -oE ':status: [0-9]+' "$BUILD/x2_client_${THREADS}_${conn}.log" | sort | uniq -c | sed 's/^/    /' >&2
        fail "the server did not serve what was asked for; a clean stream close does not establish that it did"
    fi

    printf '{"workers":%d,"conn":%d,"requested_delay":"%s","t_start":%s,"t_end":%s,' \
        "$THREADS" "$conn" "$delay" "$t_start" "$t_end" >> "$OUT"
    printf '"received":%d,"forwarded_in":%d,"forwarded_out":%d,"accepted":%d,"streams_closed":%d,"status_200":%d,"hop_ns":%d,"hop_count":%d}\n' \
        "$((a_rx - b_rx))" "$((a_in - b_in))" "$((a_out - b_out))" "$((a_acc - b_acc))" "$closed" "$ok200" "$((a_hns - b_hns))" "$((a_hn - b_hn))" >> "$OUT"

    echo "  conn $conn/$CONNECTIONS delay=$delay received=$((a_rx - b_rx)) forwarded_in=$((a_in - b_in)) accepted=$((a_acc - b_acc)) streams=$closed 200s=$ok200"
done

# Ask the capture to finish and flush before the trap deletes its namespace.
touch "$WIRE.stop"
for _ in $(seq 1 40); do
    grep -q '^#dropped' "$WIRE" 2>/dev/null && break
    sleep 0.25
done
grep -q '^#dropped' "$WIRE" 2>/dev/null || echo "  warning: the capture did not finish cleanly" >&2
rm -f "$WIRE.stop"

echo
echo "cell complete: $OUT"
exit 0
