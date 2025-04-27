#!/bin/zsh

set -e
set -o pipefail

PROJECT_DIR="$(cd "$(dirname "$0")" && pwd)"
BUILD_DIR="$PROJECT_DIR/build"
LOG_DIR="$PROJECT_DIR/log"
LB_EXECUTABLE="$BUILD_DIR/load_balancer"

# Ports
LB_PORT=8080
BACKEND_PORT=8081

# Number of test requests
NUM_REQUESTS=10

# Log files
RESPONSES_LOG="$LOG_DIR/responses.log"
SUMMARY_LOG="$LOG_DIR/summary.log"
STATUS_CODES_FILE="$LOG_DIR/status_codes.tmp"
BACKEND_LOG="$LOG_DIR/backend.log"
LB_LOG="$LOG_DIR/load_balancer.log"

# Prepare log directory
mkdir -p "$LOG_DIR"
rm -f "$RESPONSES_LOG" "$SUMMARY_LOG" "$STATUS_CODES_FILE" "$BACKEND_LOG" "$LB_LOG"

# Start backend Python server
echo ">>> Starting backend server on port $BACKEND_PORT..."
python3 -m http.server "$BACKEND_PORT" --bind 127.0.0.1 > "$BACKEND_LOG" 2>&1 &
BACKEND_PID=$!

# Give backend a second to start
sleep 1

# Start load balancer
echo ">>> Starting load balancer..."
"$LB_EXECUTABLE" > "$LB_LOG" 2>&1 &
LB_PID=$!

# Give load balancer a second to start
sleep 1

# Trap to cleanup background processes
function cleanup {
  echo ">>> Cleaning up..."
  kill $LB_PID $BACKEND_PID
  exit 0
}
trap cleanup INT TERM EXIT

echo ">>> Firing test requests at load balancer..."

for i in {1..$NUM_REQUESTS}; do
    RESPONSE_FILE="$LOG_DIR/response_$i.txt"
    
    # Fire request and capture full response
    curl -s -D - "http://127.0.0.1:$LB_PORT" -o /dev/null > "$RESPONSE_FILE" || echo "HTTP ERROR" > "$RESPONSE_FILE"
    
    # Extract HTTP status code
    STATUS_CODE=$(grep -m 1 "HTTP/" "$RESPONSE_FILE" | awk '{print $2}')
    if [[ -z "$STATUS_CODE" ]]; then
        STATUS_CODE="ERROR"
    fi

    echo "$STATUS_CODE"
    echo "Request $i: $STATUS_CODE" >> "$SUMMARY_LOG"
    echo "$STATUS_CODE" >> "$STATUS_CODES_FILE"
    cat "$RESPONSE_FILE" >> "$RESPONSES_LOG"
    echo -e "\n\n" >> "$RESPONSES_LOG"

    rm -f "$RESPONSE_FILE"
done

echo
echo ">>> Test summary:"
echo "Total Requests: $NUM_REQUESTS"

# Count and print status codes
sort "$STATUS_CODES_FILE" | uniq -c | while read count code; do
    echo "  Status $code: $count times"
done

# Cleanup
kill $LB_PID $BACKEND_PID
trap - INT TERM EXIT

echo ">>> Test complete."

# Remove temp file
rm -f "$STATUS_CODES_FILE"
