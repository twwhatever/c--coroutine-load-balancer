#!/bin/zsh

set -o pipefail

PROJECT_DIR="$(cd "$(dirname "$0")" && pwd)"
BUILD_DIR="$PROJECT_DIR/build"
LB_EXECUTABLE="$BUILD_DIR/rate_limiter"
LOG_DIR="$PROJECT_DIR/log"

mkdir -p "$LOG_DIR"

LB_CLIENT_PORT=8080
LB_CONTROL_PORT=8081
NUM_REQUESTS=5000
CONCURRENCY=200
BACKEND_PORTS=(9001 9002 9003)

RESPONSES_LOG="$LOG_DIR/stress_responses.log"
SUMMARY_LOG="$LOG_DIR/stress_summary.log"
STATUS_CODES_FILE="$LOG_DIR/stress_status_codes.tmp"
LB_LOG="$LOG_DIR/rate_limiter_stress.log"

# Clean logs
rm -f "$RESPONSES_LOG" "$SUMMARY_LOG" "$STATUS_CODES_FILE" "$LB_LOG"
for port in $BACKEND_PORTS; do
  rm -f "$LOG_DIR/backend_$port.log"
done

# Start backend servers
echo ">>> Starting backend servers..."
BACKEND_PIDS=()
for port in $BACKEND_PORTS; do
  python3 -m http.server "$port" --bind 127.0.0.1 > "$LOG_DIR/backend_$port.log" 2>&1 &
  BACKEND_PIDS+=($!)
done

sleep 1

# Start load balancer
echo ">>> Starting rate limiter (load balancer)..."
"$LB_EXECUTABLE" > "$LB_LOG" 2>&1 &
LB_PID=$!

sleep 1

# Register backends with load balancer
echo ">>> Registering backends..."
for port in $BACKEND_PORTS; do
  curl -s -X POST http://127.0.0.1:$LB_CONTROL_PORT/register \
       -H "Content-Type: application/json" \
       -d "{\"host\":\"127.0.0.1\", \"port\":$port}" > /dev/null
done

sleep 1

# Start load test
echo ">>> Starting load test with vegeta..."
echo "GET http://127.0.0.1:$LB_CLIENT_PORT" \
  | vegeta attack -duration=10s -rate=1000 \
  | tee "$LOG_DIR/results.bin" \
  | vegeta report

# Cleanup function
function cleanup {
  echo ">>> Cleaning up..."
  kill $LB_PID $BACKEND_PIDS[@] 2>/dev/null || true
  exit 0
}
trap cleanup INT TERM EXIT
