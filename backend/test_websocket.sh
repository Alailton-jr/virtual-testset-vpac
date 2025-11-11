#!/bin/bash

# test_websocket.sh - WebSocket Server Test Script
# Tests the WebSocket server running on port 8082

set -e

echo "========================================"
echo "  WebSocket Server Test"
echo "========================================"
echo ""

# Check if websocat is installed
if ! command -v websocat &> /dev/null; then
    echo "❌ websocat is not installed"
    echo "Install with: brew install websocat"
    echo ""
    echo "Alternatively, you can test with JavaScript in a browser console:"
    echo "const ws = new WebSocket('ws://localhost:8082');"
    echo "ws.onopen = () => console.log('Connected');"
    echo "ws.onmessage = (e) => console.log('Received:', e.data);"
    echo "ws.send(JSON.stringify({type: 'subscribe', topic: 'analyzer/phasors'}));"
    exit 1
fi

# Check if server is running
if ! nc -z localhost 8082 2>/dev/null; then
    echo "❌ WebSocket server is not running on port 8082"
    echo "Start the server with: sudo ./build/Main"
    exit 1
fi

echo "✅ WebSocket server is running on port 8082"
echo ""

# Test 1: Connect and receive welcome message
echo "Test 1: Connect and receive welcome message"
echo "-------------------------------------------"
timeout 2 websocat ws://localhost:8082 2>&1 | head -1 || true
echo ""

# Test 2: Subscribe to a topic
echo "Test 2: Subscribe to analyzer/phasors topic"
echo "--------------------------------------------"
echo '{"type":"subscribe","topic":"analyzer/phasors"}' | timeout 2 websocat ws://localhost:8082 2>&1 | head -3 || true
echo ""

# Test 3: Ping-pong
echo "Test 3: Ping-pong test"
echo "----------------------"
echo '{"type":"ping"}' | timeout 2 websocat ws://localhost:8082 2>&1 | head -2 || true
echo ""

# Test 4: Invalid message handling
echo "Test 4: Invalid message handling"
echo "---------------------------------"
echo '{"invalid":"json"}' | timeout 2 websocat ws://localhost:8082 2>&1 | head -2 || true
echo ""

echo "========================================"
echo "  Test complete!"
echo "========================================"
echo ""
echo "Available topics:"
echo "  - analyzer/phasors    : Live phasor updates"
echo "  - analyzer/waveforms  : Live waveform data"
echo "  - analyzer/harmonics  : Harmonics analysis"
echo "  - sequence/progress   : Test sequence state"
echo "  - goose/events        : GOOSE message events"
echo "  - stream/status       : SV stream status"
echo ""
echo "Message format:"
echo '  {"type":"subscribe","topic":"analyzer/phasors"}'
echo '  {"type":"unsubscribe","topic":"analyzer/phasors"}'
echo '  {"type":"ping"}'
echo ""
