#!/bin/bash

# Virtual TestSet - Backend Monitor Script
# This script runs the backend and displays logs in a readable format

set -e

BACKEND_DIR="$(cd "$(dirname "$0")/.." && pwd)"
BUILD_DIR="$BACKEND_DIR/build"
LOG_DIR="$BACKEND_DIR/.logs"
LOG_FILE="$LOG_DIR/backend-$(date +%Y%m%d-%H%M%S).log"

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# Create log directory
mkdir -p "$LOG_DIR"

echo -e "${GREEN}━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━${NC}"
echo -e "${GREEN}  Virtual TestSet - Backend Monitor${NC}"
echo -e "${GREEN}━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━${NC}"
echo ""
echo -e "${BLUE}Backend Directory:${NC} $BACKEND_DIR"
echo -e "${BLUE}Build Directory:${NC} $BUILD_DIR"
echo -e "${BLUE}Log File:${NC} $LOG_FILE"
echo ""

# Check if backend executable exists
if [ ! -f "$BUILD_DIR/Main" ]; then
    echo -e "${RED}✗ Backend executable not found at $BUILD_DIR/Main${NC}"
    echo ""
    echo "Building backend..."
    cd "$BACKEND_DIR"
    
    if [ -f "scripts/build_macos.sh" ]; then
        ./scripts/build_macos.sh
    else
        make build
    fi
    
    if [ ! -f "$BUILD_DIR/Main" ]; then
        echo -e "${RED}✗ Build failed. Check the output above.${NC}"
        exit 1
    fi
    echo -e "${GREEN}✓ Backend built successfully${NC}"
    echo ""
fi

# Trap Ctrl+C to clean up
trap 'echo -e "\n${YELLOW}Stopping backend...${NC}"; pkill -P $$ 2>/dev/null; exit 0' INT TERM

echo -e "${GREEN}━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━${NC}"
echo -e "${GREEN}  Starting Backend Server${NC}"
echo -e "${GREEN}━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━${NC}"
echo ""
echo -e "${YELLOW}Press Ctrl+C to stop the server${NC}"
echo ""
echo -e "${GREEN}━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━${NC}"
echo ""

# Start the backend with logging
cd "$BACKEND_DIR"
"$BUILD_DIR/Main" 2>&1 | tee "$LOG_FILE" | while IFS= read -r line; do
    # Color-code log levels
    if [[ "$line" =~ ERROR|error|Error ]]; then
        echo -e "${RED}$line${NC}"
    elif [[ "$line" =~ WARN|warn|Warning|warning ]]; then
        echo -e "${YELLOW}$line${NC}"
    elif [[ "$line" =~ INFO|info|Info|Started|Listening ]]; then
        echo -e "${GREEN}$line${NC}"
    elif [[ "$line" =~ DEBUG|debug ]]; then
        echo -e "${BLUE}$line${NC}"
    else
        echo "$line"
    fi
done

echo ""
echo -e "${YELLOW}Backend stopped.${NC}"
echo -e "${BLUE}Logs saved to:${NC} $LOG_FILE"
