#!/usr/bin/env bash
#
# Start a regtest node and mine across the height where regtest switches to
# Lyra2REv2, to check that the binaries start, serve RPC and validate blocks.
# The functional test framework does not run on this tree yet.
#
# usage: smoke_test.sh <monacoind> <monacoin-cli>

export LC_ALL=C
set -euo pipefail

DAEMON=$1
CLI=$2
TARGET_HEIGHT=70
START_TIMEOUT=300
STOP_TIMEOUT=60

LOGS="$PWD/ci-logs/smoke"
DATADIR="$PWD/ci-logs/smoke/datadir"
mkdir -p "$DATADIR"
# The Windows binaries do not understand MSYS paths.
if command -v cygpath >/dev/null; then
  DATADIR_ARG=$(cygpath -m "$DATADIR")
else
  DATADIR_ARG=$DATADIR
fi

cli() {
  "$CLI" -regtest -datadir="$DATADIR_ARG" "$@" | tr -d '\r'
}

"$DAEMON" -regtest -datadir="$DATADIR_ARG" -listen=0 -printtoconsole=0 > "$LOGS/daemon.txt" 2>&1 &
PID=$!

cleanup() {
  status=$?
  cli stop >/dev/null 2>&1 || true
  for _ in $(seq 1 "$STOP_TIMEOUT"); do
    kill -0 "$PID" 2>/dev/null || break
    sleep 1
  done
  if kill -0 "$PID" 2>/dev/null; then
    echo "::error::node did not stop within ${STOP_TIMEOUT}s"
    kill -9 "$PID" 2>/dev/null || true
    status=1
  fi
  if [ "$status" -ne 0 ]; then
    cat "$LOGS/daemon.txt" || true
    tail -n 40 "$DATADIR/regtest/debug.log" 2>/dev/null || true
  fi
  exit "$status"
}
trap cleanup EXIT

# -rpcwait gives up while the node is still warming up, so poll instead.
ready=0
for _ in $(seq 1 "$START_TIMEOUT"); do
  if ! kill -0 "$PID" 2>/dev/null; then
    echo "::error::node exited during startup"
    exit 1
  fi
  if cli getblockcount >/dev/null 2>&1; then
    ready=1
    break
  fi
  sleep 1
done
if [ "$ready" -ne 1 ]; then
  echo "::error::node did not start within ${START_TIMEOUT}s"
  exit 1
fi

ADDRESS=$(cli getnewaddress)
cli generatetoaddress "$TARGET_HEIGHT" "$ADDRESS" >/dev/null

HEIGHT=$(cli getblockcount)
if [ "$HEIGHT" != "$TARGET_HEIGHT" ]; then
  echo "::error::expected height $TARGET_HEIGHT, got '$HEIGHT'"
  exit 1
fi
echo "regtest node reached height $HEIGHT"
