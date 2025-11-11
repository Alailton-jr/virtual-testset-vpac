#!/bin/bash

# Quick E2E Test Runner for Virtual TestSet
# This script runs the E2E test suite with Docker Compose

set -e

echo "🚀 Virtual TestSet - E2E Test Runner"
echo "===================================="
echo ""

# Check if we're in the right directory
if [ ! -f "tests/e2e/package.json" ]; then
    echo "❌ Error: Must be run from project root"
    echo "Current directory: $(pwd)"
    exit 1
fi

# Check if dependencies are installed
if [ ! -d "tests/e2e/node_modules" ]; then
    echo "📦 Installing E2E test dependencies..."
    cd tests/e2e
    npm install
    npx playwright install chromium
    cd ../..
    echo "✅ Dependencies installed"
    echo ""
fi

# Display menu
echo "Select test mode:"
echo "1) Run all tests (headless)"
echo "2) Run all tests (headed - show browser)"
echo "3) Run with Playwright UI"
echo "4) Run specific test file"
echo "5) View last test report"
echo "6) Clean up and exit"
echo ""
read -p "Enter choice [1-6]: " choice

case $choice in
    1)
        echo "🧪 Running all E2E tests (headless)..."
        cd tests/e2e
        npm test
        ;;
    2)
        echo "🧪 Running all E2E tests (headed mode)..."
        cd tests/e2e
        npm run test:headed
        ;;
    3)
        echo "🎨 Opening Playwright UI..."
        cd tests/e2e
        npm run test:ui
        ;;
    4)
        echo ""
        echo "Available test files:"
        echo "  1) 01-streams.spec.ts - Stream Management"
        echo "  2) 02-manual-injection.spec.ts - Phasor Control"
        echo "  3) 03-comtrade.spec.ts - COMTRADE Playback"
        echo "  4) 04-sequencer.spec.ts - Sequence Builder"
        echo "  5) 05-analyzer.spec.ts - Real-Time Analyzer"
        echo "  6) 06-navigation.spec.ts - Navigation & Error Handling"
        echo ""
        read -p "Enter test number [1-6]: " test_num
        
        case $test_num in
            1) test_file="01-streams.spec.ts" ;;
            2) test_file="02-manual-injection.spec.ts" ;;
            3) test_file="03-comtrade.spec.ts" ;;
            4) test_file="04-sequencer.spec.ts" ;;
            5) test_file="05-analyzer.spec.ts" ;;
            6) test_file="06-navigation.spec.ts" ;;
            *) echo "❌ Invalid choice"; exit 1 ;;
        esac
        
        echo "🧪 Running $test_file..."
        cd tests/e2e
        npx playwright test "tests/$test_file"
        ;;
    5)
        echo "📊 Opening test report..."
        cd tests/e2e
        npm run report
        ;;
    6)
        echo "🧹 Cleaning up Docker services..."
        cd backend
        docker compose down -v
        echo "✅ Cleanup complete"
        exit 0
        ;;
    *)
        echo "❌ Invalid choice"
        exit 1
        ;;
esac

echo ""
echo "✅ Tests complete!"
echo ""
echo "📊 To view the HTML report, run:"
echo "   cd tests/e2e && npm run report"
echo ""
echo "🧹 To stop Docker services, run:"
echo "   cd backend && docker compose down -v"
