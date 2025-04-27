#!/bin/zsh

set -o pipefail

PROJECT_DIR="$(cd "$(dirname "$0")" && pwd)"
BUILD_DIR="$PROJECT_DIR/build"
LB_EXECUTABLE="$BUILD_DIR/rate_limiter"
LOG_DIR="$PROJECT_DIR/log"

mkdir -p "$LOG_DIR"

LB_PORT=8080
BACKEND_PORT=8081
NUM_REQUESTS=5000
CONCURRENCY=200

RESPONSES_LOG="$LOG_DIR/stress_responses.log"
SUMMARY_LOG="$LOG_DIR/stress_summary.log"
STATUS_CODES_FILE="$LOG_DIR/stress_status_codes.tmp"
BACKEND_LOG="$LOG_DIR/backend_stress.log"
LB_LOG="$LOG_DIR/rate_limiter_stress.log"

# Clean logs
rm -f "$RESPONSES_LOG" "$SUMMARY_LOG" "$STATUS_CODES_FILE" "$BACKEND_LOG" "$LB_LOG"

# Start backend
echo ">>> Starting backend server on port $BACKEND_PORT..."
python3 -m http.server "$BACKEND_PORT" --bind 127.0.0.1 > "$BACKEND_LOG" 2>&1 &
BACKEND_PID=$!

sleep 1

# Start load balancer
echo ">>> Starting rate limiter..."
"$LB_EXECUTABLE" > "$LB_LOG" 2>&1 &
LB_PID=$!

sleep 1

# Start load test
echo ">>> Starting load test..."
echo "GET http://127.0.0.1:$LB_PORT" | vegeta attack -duration=10s -rate=1000 | tee $LOG_DIR/results.bin | vegeta report

# Cleanup function
function cleanup {
  echo ">>> Cleaning up..."
  kill $LB_PID $BACKEND_PID
  exit 0
}
trap cleanup INT TERM EXIT
