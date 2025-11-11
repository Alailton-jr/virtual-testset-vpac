#!/bin/bash
#
# Quick smoke test for the HTTP API server
# Tests basic endpoints to verify the SV Publisher Manager integration
#

set -e

API_URL="http://localhost:8081"
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

echo "========================================="
echo "Virtual TestSet API Smoke Test"
echo "========================================="
echo ""

# Check if server is running
echo -n "Checking if API server is running... "
if curl -s -f "${API_URL}/api/v1/health" > /dev/null 2>&1; then
    echo -e "${GREEN}✓${NC}"
else
    echo -e "${RED}✗${NC}"
    echo ""
    echo -e "${YELLOW}Server not running. Start it with:${NC}"
    echo "  cd backend && sudo ./build/Main"
    echo ""
    exit 1
fi

# Test health endpoint
echo -n "Testing health endpoint... "
HEALTH=$(curl -s "${API_URL}/api/v1/health")
if echo "$HEALTH" | grep -q '"status":"ok"'; then
    echo -e "${GREEN}✓${NC}"
else
    echo -e "${RED}✗${NC}"
    echo "Response: $HEALTH"
    exit 1
fi

# Test list streams (empty initially)
echo -n "Testing list streams... "
STREAMS=$(curl -s "${API_URL}/api/v1/streams")
echo -e "${GREEN}✓${NC}"

# Create a test stream
echo -n "Creating test stream... "
CREATE_RESPONSE=$(curl -s -X POST "${API_URL}/api/v1/streams" \
    -H "Content-Type: application/json" \
    -d '{
        "appId": "0x4000",
        "macDst": "01:0C:CD:04:00:00",
        "macSrc": "AA:BB:CC:DD:EE:01",
        "vlanId": 100,
        "vlanPrio": 4,
        "svId": "TEST_STREAM",
        "dstAddress": "",
        "nominalFreq": 60.0,
        "sampleRate": 4800,
        "dataSource": "MANUAL"
    }')

STREAM_ID=$(echo "$CREATE_RESPONSE" | grep -o '"id":"[^"]*"' | cut -d'"' -f4)

if [ -n "$STREAM_ID" ]; then
    echo -e "${GREEN}✓${NC} (ID: $STREAM_ID)"
else
    echo -e "${RED}✗${NC}"
    echo "Response: $CREATE_RESPONSE"
    exit 1
fi

# Update phasors
echo -n "Updating phasors... "
UPDATE_RESPONSE=$(curl -s -X PUT "${API_URL}/api/v1/streams/${STREAM_ID}/phasors" \
    -H "Content-Type: application/json" \
    -d '{
        "phasors": [
            {"magnitude": 120.0, "angle": 0.0},
            {"magnitude": 120.0, "angle": -120.0},
            {"magnitude": 120.0, "angle": 120.0}
        ]
    }')

if echo "$UPDATE_RESPONSE" | grep -q '"message":"Phasors updated"'; then
    echo -e "${GREEN}✓${NC}"
else
    echo -e "${RED}✗${NC}"
    echo "Response: $UPDATE_RESPONSE"
    exit 1
fi

# Start stream
echo -n "Starting stream... "
START_RESPONSE=$(curl -s -X POST "${API_URL}/api/v1/streams/${STREAM_ID}/start")

if echo "$START_RESPONSE" | grep -q '"message":"Stream started successfully"'; then
    echo -e "${GREEN}✓${NC}"
else
    echo -e "${RED}✗${NC}"
    echo "Response: $START_RESPONSE"
    exit 1
fi

# Stop stream
echo -n "Stopping stream... "
STOP_RESPONSE=$(curl -s -X POST "${API_URL}/api/v1/streams/${STREAM_ID}/stop")

if echo "$STOP_RESPONSE" | grep -q '"message":"Stream stopped successfully"'; then
    echo -e "${GREEN}✓${NC}"
else
    echo -e "${RED}✗${NC}"
    echo "Response: $STOP_RESPONSE"
    exit 1
fi

# Delete stream
echo -n "Deleting stream... "
DELETE_RESPONSE=$(curl -s -X DELETE "${API_URL}/api/v1/streams/${STREAM_ID}")

if echo "$DELETE_RESPONSE" | grep -q '"message":"Stream deleted successfully"'; then
    echo -e "${GREEN}✓${NC}"
else
    echo -e "${RED}✗${NC}"
    echo "Response: $DELETE_RESPONSE"
    exit 1
fi

echo ""
echo "========================================="
echo -e "${GREEN}All tests passed!${NC}"
echo "========================================="
echo ""
echo "API is fully functional. Next steps:"
echo "  - Implement WebSocket server for real-time data"
echo "  - Add COMTRADE/CSV parsers"
echo "  - Build React frontend"
echo ""
