#!/bin/bash

# Virtual TestSet - Backend Status Checker
# Quick check if backend is running and accessible

# Colors
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m'

BACKEND_PORT=${BACKEND_PORT:-8080}
BACKEND_HOST=${BACKEND_HOST:-localhost}

echo -e "${BLUE}━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━${NC}"
echo -e "${BLUE}  Virtual TestSet - Backend Status${NC}"
echo -e "${BLUE}━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━${NC}"
echo ""

# Check if port is in use
if lsof -Pi :$BACKEND_PORT -sTCP:LISTEN -t >/dev/null 2>&1; then
    PID=$(lsof -Pi :$BACKEND_PORT -sTCP:LISTEN -t)
    PROCESS=$(ps -p $PID -o comm=)
    echo -e "${GREEN}✓ Process running on port $BACKEND_PORT${NC}"
    echo -e "  PID: $PID"
    echo -e "  Process: $PROCESS"
else
    echo -e "${RED}✗ No process listening on port $BACKEND_PORT${NC}"
    echo ""
    echo "To start the backend:"
    echo -e "${YELLOW}  cd backend && ./scripts/monitor.sh${NC}"
    exit 1
fi

echo ""

# Check HTTP endpoint
echo -e "${BLUE}Testing HTTP endpoint...${NC}"
if curl -s -f -m 5 "http://$BACKEND_HOST:$BACKEND_PORT/api/health" >/dev/null 2>&1; then
    echo -e "${GREEN}✓ HTTP API is responding${NC}"
    RESPONSE=$(curl -s -m 5 "http://$BACKEND_HOST:$BACKEND_PORT/api/health")
    echo -e "  Response: $RESPONSE"
elif curl -s -f -m 5 "http://$BACKEND_HOST:$BACKEND_PORT/" >/dev/null 2>&1; then
    echo -e "${GREEN}✓ HTTP server is responding${NC}"
else
    echo -e "${YELLOW}⚠ HTTP server not responding (may still be starting)${NC}"
fi

echo ""

# Check recent logs
LOG_DIR="$(dirname "$0")/../.logs"
if [ -d "$LOG_DIR" ]; then
    LATEST_LOG=$(ls -t "$LOG_DIR"/backend-*.log 2>/dev/null | head -n 1)
    if [ -n "$LATEST_LOG" ]; then
        echo -e "${BLUE}Recent log entries (last 10 lines):${NC}"
        echo -e "${YELLOW}────────────────────────────────────────────────────────${NC}"
        tail -n 10 "$LATEST_LOG" | while IFS= read -r line; do
            if [[ "$line" =~ ERROR|error ]]; then
                echo -e "${RED}$line${NC}"
            elif [[ "$line" =~ WARN|warn ]]; then
                echo -e "${YELLOW}$line${NC}"
            else
                echo "$line"
            fi
        done
        echo -e "${YELLOW}────────────────────────────────────────────────────────${NC}"
        echo ""
        echo -e "${BLUE}Full logs:${NC} $LATEST_LOG"
    fi
fi

echo ""
echo -e "${BLUE}━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━${NC}"

# Show available commands
echo ""
echo -e "${BLUE}Available commands:${NC}"
echo ""
echo -e "  ${GREEN}./scripts/watch-logs.sh${NC}     - Watch logs in real-time"
echo -e "  ${GREEN}./scripts/monitor.sh${NC}        - Start backend with monitoring"
echo -e "  ${GREEN}kill $PID${NC}                   - Stop the backend"
echo ""
