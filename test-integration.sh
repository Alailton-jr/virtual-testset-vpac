#!/bin/bash
# Integration Test Script for Virtual TestSet
# Tests the full stack: Backend API + Frontend

set -e  # Exit on any error

echo "=================================================="
echo "Virtual TestSet - Integration Test Suite"
echo "=================================================="
echo ""

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

# Test counter
TESTS_PASSED=0
TESTS_FAILED=0

# Helper functions
pass() {
    echo -e "${GREEN}✓${NC} $1"
    ((TESTS_PASSED++))
}

fail() {
    echo -e "${RED}✗${NC} $1"
    ((TESTS_FAILED++))
}

info() {
    echo -e "${YELLOW}ℹ${NC} $1"
}

# Check prerequisites
echo "=== Phase 1: Prerequisites Check ==="
echo ""

if command -v docker &> /dev/null; then
    pass "Docker is installed"
else
    fail "Docker is not installed"
    exit 1
fi

if command -v curl &> /dev/null; then
    pass "curl is installed"
else
    fail "curl is not installed"
    exit 1
fi

echo ""

# Check if backend is built locally
echo "=== Phase 2: Local Backend Check ==="
echo ""

if [ -f "backend/build/Main" ]; then
    pass "Backend binary exists"
else
    fail "Backend binary not found - run: cd backend && cmake -B build && cmake --build build"
fi

if [ -f "backend/build/vts_tests" ]; then
    pass "Backend test binary exists"
else
    fail "Backend test binary not found"
fi

echo ""

# Check frontend build
echo "=== Phase 3: Frontend Build Check ==="
echo ""

if [ -d "frontend/dist" ]; then
    pass "Frontend dist directory exists"
    if [ -f "frontend/dist/index.html" ]; then
        pass "Frontend index.html exists"
    else
        fail "Frontend index.html not found"
    fi
else
    info "Frontend not built - run: cd frontend && npm run build"
fi

echo ""

# Test backend unit tests
echo "=== Phase 4: Backend Unit Tests ==="
echo ""

if [ -f "backend/build/vts_tests" ]; then
    info "Running backend unit tests..."
    cd backend/build
    if ./vts_tests 2>&1 | grep -q "PASSED"; then
        pass "Backend unit tests passed"
    else
        fail "Backend unit tests failed"
    fi
    cd ../..
else
    fail "Cannot run unit tests - binary not found"
fi

echo ""

# API smoke tests (if backend is running)
echo "=== Phase 5: API Smoke Tests ==="
echo ""

info "Checking if backend is running on port 8080..."

if curl -s http://localhost:8080/api/v1/health > /dev/null 2>&1; then
    pass "Backend is running and responsive"
    
    # Test health endpoint
    HEALTH_RESPONSE=$(curl -s http://localhost:8080/api/v1/health)
    if echo "$HEALTH_RESPONSE" | grep -q "status"; then
        pass "Health endpoint returns valid JSON"
    else
        fail "Health endpoint response invalid"
    fi
    
    # Test streams endpoint
    STREAMS_RESPONSE=$(curl -s http://localhost:8080/api/v1/streams)
    if echo "$STREAMS_RESPONSE" | grep -q "streams"; then
        pass "Streams endpoint accessible"
    else
        fail "Streams endpoint not working"
    fi
    
else
    info "Backend not running - skipping API tests"
    info "To start backend: cd backend/build && ./Main --api-port 8080 --ws-port 8090"
fi

echo ""

# Frontend smoke test (if running)
echo "=== Phase 6: Frontend Smoke Tests ==="
echo ""

info "Checking if frontend is running on port 5173..."

if curl -s http://localhost:5173 > /dev/null 2>&1; then
    pass "Frontend is running and responsive"
    
    # Check if it's the React app
    FRONTEND_RESPONSE=$(curl -s http://localhost:5173)
    if echo "$FRONTEND_RESPONSE" | grep -q "virtual.*test.*set\|vts\|Virtual TestSet" -i; then
        pass "Frontend returns VTS application"
    else
        info "Frontend response doesn't match expected content"
    fi
else
    info "Frontend not running - skipping frontend tests"
    info "To start frontend: cd frontend && npm run dev"
fi

echo ""

# Docker compose validation
echo "=== Phase 7: Docker Compose Validation ==="
echo ""

if [ -f "docker/docker-compose.yml" ]; then
    pass "docker-compose.yml exists"
    
    # Validate compose file
    cd docker
    if docker compose config > /dev/null 2>&1; then
        pass "docker-compose.yml is valid"
    else
        fail "docker-compose.yml has errors"
    fi
    cd ..
else
    fail "docker-compose.yml not found"
fi

if [ -f "docker/Dockerfile.backend" ]; then
    pass "Dockerfile.backend exists"
else
    fail "Dockerfile.backend not found"
fi

if [ -f "docker/Dockerfile.frontend" ]; then
    pass "Dockerfile.frontend exists"
else
    fail "Dockerfile.frontend not found"
fi

if [ -f "docker/nginx.conf" ]; then
    pass "nginx.conf exists"
else
    fail "nginx.conf not found"
fi

echo ""

# Summary
echo "=================================================="
echo "Integration Test Summary"
echo "=================================================="
echo ""
echo -e "Tests Passed: ${GREEN}${TESTS_PASSED}${NC}"
echo -e "Tests Failed: ${RED}${TESTS_FAILED}${NC}"
echo ""

if [ $TESTS_FAILED -eq 0 ]; then
    echo -e "${GREEN}✓ All tests passed!${NC}"
    echo ""
    echo "Next steps:"
    echo "1. Start the system: cd docker && docker compose --profile dev up --build"
    echo "2. Access frontend: http://localhost:5173"
    echo "3. Run full integration tests from INTEGRATION_VERIFICATION.md"
    exit 0
else
    echo -e "${RED}✗ Some tests failed${NC}"
    echo ""
    echo "Please fix the failing tests before proceeding with integration."
    exit 1
fi
