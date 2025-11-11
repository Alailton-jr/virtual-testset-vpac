#!/bin/bash

# Virtual TestSet - Log Viewer
# Watch backend logs in real-time with color coding

LOG_DIR="$(cd "$(dirname "$0")/../.logs" && pwd 2>/dev/null || echo "$(dirname "$0")/../.logs")"

# Colors
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
MAGENTA='\033[0;35m'
CYAN='\033[0;36m'
NC='\033[0m' # No Color

echo -e "${CYAN}━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━${NC}"
echo -e "${CYAN}  Virtual TestSet - Backend Log Viewer${NC}"
echo -e "${CYAN}━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━${NC}"
echo ""

# Check if log directory exists
if [ ! -d "$LOG_DIR" ]; then
    echo -e "${YELLOW}No logs directory found. Backend hasn't been started yet.${NC}"
    echo ""
    echo "To start the backend with monitoring, run:"
    echo -e "${GREEN}  cd backend && ./scripts/monitor.sh${NC}"
    exit 1
fi

# Find the most recent log file
LATEST_LOG=$(ls -t "$LOG_DIR"/backend-*.log 2>/dev/null | head -n 1)

if [ -z "$LATEST_LOG" ]; then
    echo -e "${YELLOW}No log files found in $LOG_DIR${NC}"
    echo ""
    echo "To start the backend with monitoring, run:"
    echo -e "${GREEN}  cd backend && ./scripts/monitor.sh${NC}"
    exit 1
fi

echo -e "${BLUE}Watching:${NC} $LATEST_LOG"
echo -e "${YELLOW}Press Ctrl+C to exit${NC}"
echo ""
echo -e "${CYAN}━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━${NC}"
echo ""

# Watch the log file with color coding
tail -f "$LATEST_LOG" | while IFS= read -r line; do
    if [[ "$line" =~ ERROR|error|Error|FATAL|fatal ]]; then
        echo -e "${RED}$line${NC}"
    elif [[ "$line" =~ WARN|warn|Warning|warning ]]; then
        echo -e "${YELLOW}$line${NC}"
    elif [[ "$line" =~ INFO|info|Info ]]; then
        echo -e "${GREEN}$line${NC}"
    elif [[ "$line" =~ DEBUG|debug ]]; then
        echo -e "${BLUE}$line${NC}"
    elif [[ "$line" =~ Started|Listening|Success|success ]]; then
        echo -e "${GREEN}✓ $line${NC}"
    elif [[ "$line" =~ Failed|failed|Fail ]]; then
        echo -e "${RED}✗ $line${NC}"
    elif [[ "$line" =~ WebSocket|websocket|WS ]]; then
        echo -e "${MAGENTA}$line${NC}"
    elif [[ "$line" =~ HTTP|http|GET|POST|PUT|DELETE|PATCH ]]; then
        echo -e "${CYAN}$line${NC}"
    else
        echo "$line"
    fi
done
