#!/usr/bin/env bash
#
# Sync a node from nothing, check that it reached the tip, then erase the chain.
#
# The chain goes in a new temporary directory under DATA_ROOT, so an existing
# datadir there is never used or erased. Exits 0 only if the sync completed.
#
# Environment:
#   DATA_ROOT      where to create the temporary datadir (default: /tmp)
#   SYNC_TIMEOUT   give up after this many seconds (default: 21000)
#   STALL_TIMEOUT  give up if no new block or header arrives for this many
#                  seconds (default: 1800)
#   POLL_INTERVAL  seconds between checks (default: 60)
#   MIN_HEIGHT     the tip must be at least this high (default: 4100000)
#   CHECK_BLOCK    "height:hash" that must be in the synced chain, or empty
#   KEEP_CHAIN     set to 1 to keep the chain, for debugging
#   EXTRA_ARGS     options for both monacoind and monacoin-cli, like -regtest

export LC_ALL=C
set -euo pipefail

DATA_ROOT=${DATA_ROOT:-/tmp}
SYNC_TIMEOUT=${SYNC_TIMEOUT:-21000}
STALL_TIMEOUT=${STALL_TIMEOUT:-1800}
POLL_INTERVAL=${POLL_INTERVAL:-60}
MIN_HEIGHT=${MIN_HEIGHT:-4100000}
CHECK_BLOCK=${CHECK_BLOCK-4100000:f822607c1f5d87a8a5183188b27d10b1de8b58d086152620cdf1efc4919e152e}
KEEP_CHAIN=${KEEP_CHAIN:-0}
EXTRA_ARGS=${EXTRA_ARGS:-}
DAEMON=${MONACOIND:-monacoind}
CLI=${MONACOIN_CLI:-monacoin-cli}

DATADIR=$(mktemp -d "$DATA_ROOT/sync-test.XXXXXXXX")
PID=

# shellcheck disable=SC2086
cli() {
  "$CLI" $EXTRA_ARGS -datadir="$DATADIR" "$@"
}

cleanup() {
  status=$?
  trap - EXIT INT TERM
  if [ -n "$PID" ] && kill -0 "$PID" 2>/dev/null; then
    cli stop >/dev/null 2>&1 || kill "$PID" 2>/dev/null || true
    for _ in $(seq 1 300); do
      kill -0 "$PID" 2>/dev/null || break
      sleep 1
    done
    kill -9 "$PID" 2>/dev/null || true
  fi
  if [ "$status" -ne 0 ]; then
    echo "--- last lines of debug.log"
    find "$DATADIR" -name debug.log -exec tail -n 40 {} \; 2>/dev/null || true
  fi
  if [ "$KEEP_CHAIN" = 1 ]; then
    echo "chain kept in $DATADIR"
  else
    rm -rf "$DATADIR"
    echo "chain erased"
  fi
  echo "result: $([ "$status" -eq 0 ] && echo PASS || echo FAIL)"
  exit "$status"
}
trap cleanup EXIT
trap 'exit 130' INT
trap 'exit 143' TERM

fail() {
  echo "FAIL: $*"
  exit 1
}

echo "version: $("$DAEMON" -version | head -n 1)"

# shellcheck disable=SC2086
"$DAEMON" $EXTRA_ARGS -datadir="$DATADIR" -disablewallet -listen=0 -printtoconsole=0 >/dev/null 2>&1 &
PID=$!

start=$(date +%s)
last_progress=$start
last_blocks=-1
last_headers=-1
while :; do
  kill -0 "$PID" 2>/dev/null || fail "node exited unexpectedly"
  now=$(date +%s)
  [ $((now - start)) -lt "$SYNC_TIMEOUT" ] || fail "not synced after ${SYNC_TIMEOUT}s"
  [ $((now - last_progress)) -lt "$STALL_TIMEOUT" ] || fail "no progress for ${STALL_TIMEOUT}s"

  if info=$(cli getblockchaininfo 2>/dev/null); then
    blocks=$(jq -r .blocks <<<"$info")
    headers=$(jq -r .headers <<<"$info")
    ibd=$(jq -r .initialblockdownload <<<"$info")
    progress=$(jq -r .verificationprogress <<<"$info")
    peers=$(cli getconnectioncount 2>/dev/null || echo "?")

    if [ "$blocks" -ne "$last_blocks" ] || [ "$headers" -ne "$last_headers" ]; then
      last_progress=$now
      last_blocks=$blocks
      last_headers=$headers
    fi
    echo "$((now - start))s blocks=$blocks headers=$headers progress=$progress peers=$peers ibd=$ibd"

    if [ "$ibd" = false ] && [ "$blocks" -eq "$headers" ] && [ "$blocks" -ge "$MIN_HEIGHT" ]; then
      break
    fi
  else
    # Still warming up, which is not progress.
    echo "$((now - start))s waiting for RPC"
  fi
  # In the background, so that a signal is handled at once rather than after
  # the sleep.
  sleep "$POLL_INTERVAL" &
  wait $!
done

if [ -n "$CHECK_BLOCK" ]; then
  height=${CHECK_BLOCK%%:*}
  want=${CHECK_BLOCK#*:}
  got=$(cli getblockhash "$height")
  [ "$got" = "$want" ] || fail "block $height is $got, expected $want"
  echo "block $height matches $want"
fi

tip=$(cli getbestblockhash)
echo "synced to height $blocks, tip $tip, in $(($(date +%s) - start))s"
du -sh "$DATADIR" | cut -f1 | sed 's/^/size on disk: /'
