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
LOG="$LOG_DIR/http3_path_migration.log"

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
# Two phases, because two things called "migration" are not the same thing, and only one
# of them exercises identifier-keyed lookup. Established 3 September by running both
# against both trees:
#
#   code             rebinding phase   client-initiated phase
#   before the fix   pass              FAIL, three Stateless Resets
#   after the fix    pass              pass
#
# Address rebinding, as a NAT does it, is invisible to the client: it never learns it
# moved, so it never switches destination connection ID, so the endpoint's original map
# entry still resolves. It exercises the four-tuple hash miss and the forwarding path and
# nothing else. Client-initiated migration (RFC 9000 section 9.5) requires a fresh
# destination CID and path validation, and only it reaches the case where a previously
# advertised identifier must already be a lookup key.
#
# So the client-initiated phase runs first and is the primary one: the defect in which
# additional advertised CIDs never became lookup keys, and a migrating client was answered
# with a Stateless Reset that tore down its live connection, is invisible to rebinding
# alone. A test that performs only rebinding passes on the broken tree. This one does not.
# Both phases must pass, and they are reported separately, because they cover different
# paths and a single verdict would hide which one moved.
MAX_ATTEMPTS=12

# run_phase <mode> <label>
#   mode=client   the client migrates itself, with path validation and a fresh CID
#   mode=rebind   nft SNAT remaps the source port under a client that never notices
#
# Sets PHASE_CLIENT_OK and PHASE_FORWARDED. Each attempt is a fresh connection; several
# are tried because the kernel's four-tuple hash is not under our control and a remapped
# address can still land on the owning worker, which is the 1/N case rather than a failure.
run_phase() {
    local mode="$1" label="$2"
    local attempt=0 snat_port=40100
    PHASE_CLIENT_OK=0
    PHASE_FORWARDED=0
    PHASE_ATTEMPTS=0

    echo
    echo "######## phase: $label ########"

    while [ "$attempt" -lt "$MAX_ATTEMPTS" ]; do
        attempt=$((attempt + 1))
        snat_port=$((snat_port + 17))
        PHASE_ATTEMPTS=$attempt
        echo
        if [ "$mode" = rebind ]; then
            echo "== $label attempt $attempt/$MAX_ATTEMPTS (SNAT sport $snat_port) =="
        else
            echo "== $label attempt $attempt/$MAX_ATTEMPTS (client changes its own address) =="
        fi

        local before before_out before_in
        before="$(read_stats)"
        before_out="$(printf '%s\n' "$before" | stat_field forwarded_out)"
        before_in="$(printf '%s\n' "$before" | stat_field forwarded_in)"

        # Clear prior NAT mappings either way: in the client phase the chain must be empty
        # or a rule left by the rebinding phase would move the address for us, and the
        # phase would silently become the other one.
        "${RUN_IN_NS[@]}" "$CLIENT_NS" nft flush chain ip coroute_mig postrouting

        local client_log="$BUILD/http3_mig_client_${mode}_${attempt}.log"
        local -a extra=()
        if [ "$mode" = client ]; then
            # The client moves at 1s, the request opens at 2s, so the stream is carried
            # entirely on the new path and with the new connection ID.
            extra+=(--change-local-addr=1s)
        fi

        "${RUN_IN_NS[@]}" "$CLIENT_NS" env LD_LIBRARY_PATH="$LD_LIBRARY_PATH" \
            timeout 25 "$CLIENT" \
            --delay-stream=2s \
            "${extra[@]}" \
            --exit-on-first-stream-close \
            "$SERVER_IP" "$PORT" "https://$SERVER_IP:$PORT/" \
            > "$client_log" 2>&1 &
        CLIENT_PID=$!

        if [ "$mode" = rebind ]; then
            # Let the handshake finish, then remap under it.
            sleep 0.8
            apply_snat "$snat_port"
        fi

        wait "$CLIENT_PID" || true
        local client_status=$?
        CLIENT_PID=

        # A clean stream close is the survival signal and subsumes ":status: 200" -- the
        # stream cannot close with no error unless the headers, the body and the FIN all
        # arrived. The body match is a contiguous fragment because the client's hex dump
        # wraps the ASCII column at 16 and splits "coroute".
        if grep -qF "Negotiated ALPN is h3" "$client_log" \
            && grep -qF "oute h3 ok" "$client_log" \
            && grep -qE 'HTTP stream 0x0 closed with error codes \(RX:\(no error\)' "$client_log"; then
            echo "  client: request completed after the move"
            PHASE_CLIENT_OK=1
        else
            echo "  client: incomplete (exit=$client_status); checking fragments:"
            grep -n 'ALPN\|status\|h3 ok\|stream 0x0 closed\|stateless' "$client_log" | tail -20 | sed 's/^/    /' || true
            tail -10 "$client_log" | sed 's/^/    /'
        fi

        local after after_out after_in
        after="$(read_stats)"
        echo "$after" | sed 's/^/  /'
        after_out="$(printf '%s\n' "$after" | stat_field forwarded_out)"
        after_in="$(printf '%s\n' "$after" | stat_field forwarded_in)"
        echo "  delta forwarded_out=$((after_out - before_out)) forwarded_in=$((after_in - before_in))"

        if [ "$((after_out - before_out))" -gt 0 ] || [ "$((after_in - before_in))" -gt 0 ]; then
            PHASE_FORWARDED=1
            echo "  userspace forwarding fired"
            break
        fi
    done
}

# The primary phase. This is the one that fails on the parent of the fix.
run_phase client "client-initiated migration (fresh CID, path validation)"
CLIENT_PHASE_OK=$PHASE_CLIENT_OK
CLIENT_PHASE_FWD=$PHASE_FORWARDED
CLIENT_PHASE_ATTEMPTS=$PHASE_ATTEMPTS

# The second phase, named for the path it actually covers.
run_phase rebind "address rebinding (same CID, four-tuple hash miss only)"
REBIND_PHASE_OK=$PHASE_CLIENT_OK
REBIND_PHASE_FWD=$PHASE_FORWARDED
REBIND_PHASE_ATTEMPTS=$PHASE_ATTEMPTS

echo
echo "== final quic-stats =="
FINAL="$(read_stats)"
echo "$FINAL" | sed 's/^/  /'
FINAL_OUT="$(printf '%s\n' "$FINAL" | stat_field forwarded_out)"
FINAL_IN="$(printf '%s\n' "$FINAL" | stat_field forwarded_in)"
FINAL_RX="$(printf '%s\n' "$FINAL" | stat_field received)"
FINAL_ACC="$(printf '%s\n' "$FINAL" | stat_field accepted)"
FINAL_SR="$(printf '%s\n' "$FINAL" | stat_field stateless_resets)"

echo
echo "workers=$THREADS"
echo "client_migration_ok=$CLIENT_PHASE_OK"
echo "client_migration_forwarded=$CLIENT_PHASE_FWD"
echo "client_migration_attempts=$CLIENT_PHASE_ATTEMPTS"
echo "rebinding_ok=$REBIND_PHASE_OK"
echo "rebinding_forwarded=$REBIND_PHASE_FWD"
echo "rebinding_attempts=$REBIND_PHASE_ATTEMPTS"
echo "forwarded_out=$FINAL_OUT"
echo "forwarded_in=$FINAL_IN"
echo "received=$FINAL_RX"
echo "accepted=$FINAL_ACC"
echo "stateless_resets=$FINAL_SR"
echo "log=$LOG"

# Reported separately: the phases cover different paths, and one verdict would hide which.
if [ "$CLIENT_PHASE_OK" -ne 1 ]; then
    echo "--- server log ---" >&2
    cat "$BUILD/http3_mig_server.log" >&2
    fail "client-initiated migration: connection did not survive a fresh destination CID. \
This is the shape the pre-fix tree failed with; check stateless_resets above."
fi
if [ "$REBIND_PHASE_OK" -ne 1 ]; then
    echo "--- server log ---" >&2
    cat "$BUILD/http3_mig_server.log" >&2
    fail "address rebinding: connection did not survive a remapped four-tuple"
fi
if [ "$CLIENT_PHASE_FWD" -ne 1 ] && [ "$REBIND_PHASE_FWD" -ne 1 ]; then
    echo "--- server log ---" >&2
    cat "$BUILD/http3_mig_server.log" >&2
    fail "forwarded_out/forwarded_in never incremented in either phase (forwarding did not fire)"
fi

echo
echo "HTTP/3 path migration: both phases survived; userspace CID forwarding fired."
exit 0
