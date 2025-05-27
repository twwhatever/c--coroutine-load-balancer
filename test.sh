#!/bin/zsh

set -euo pipefail

# Config
BACKEND_PORT=9000
LB_CLIENT_PORT=8080
LB_CONTROL_PORT=8081
LOG_DIR=log
mkdir -p "$LOG_DIR"

# Start backend server
echo ">>> Starting backend server on port $BACKEND_PORT..."
python3 -m http.server "$BACKEND_PORT" --bind 127.0.0.1 > "$LOG_DIR/backend.log" 2>&1 &
BACKEND_PID=$!
sleep 1

# Start load balancer
echo ">>> Starting load balancer..."
./build/rate_limiter > "$LOG_DIR/load_balancer.log" 2>&1 &
LB_PID=$!
sleep 1

# Register backend
echo ">>> Registering backend with load balancer..."
curl -s -X POST "http://127.0.0.1:$LB_CONTROL_PORT/register" \
     -H "Content-Type: application/json" \
     -d "{\"host\":\"127.0.0.1\", \"port\":$BACKEND_PORT}" || {
  echo ">>> ERROR: Failed to register backend"
  kill $BACKEND_PID $LB_PID 2>/dev/null || true
  exit 1
}
sleep 1

# Fire test requests to load balancer
echo ">>> Firing test requests at load balancer..."
STATUS_CODES_FILE="$LOG_DIR/status_codes.log"
RESPONSES_LOG="$LOG_DIR/responses.log"
rm -f "$STATUS_CODES_FILE" "$RESPONSES_LOG"

for i in {1..10}; do
  STATUS=$(curl -s -o /dev/null -w "%{http_code}" "http://127.0.0.1:$LB_CLIENT_PORT")
  echo "$STATUS" >> "$STATUS_CODES_FILE"
done

# Summarize
echo
echo ">>> Test summary:"
TOTAL=$(wc -l < "$STATUS_CODES_FILE")
echo "Total Requests: $TOTAL"
sort "$STATUS_CODES_FILE" | uniq -c | awk '{printf "  Status %s: %s times\n", $2, $1}'
echo ">>> Test complete."

# Cleanup
echo ">>> Cleaning up..."
kill $BACKEND_PID $LB_PID 2>/dev/null || true
