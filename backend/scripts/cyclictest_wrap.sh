#!/bin/bash
# Phase 9: Cyclictest wrapper for real-time latency testing
# Measures scheduler latency to validate RT performance

set -e

# Color output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

echo -e "${BLUE}=== Real-Time Latency Testing (cyclictest) ===${NC}"
echo ""

# Check if cyclictest is installed
if ! command -v cyclictest &> /dev/null; then
    echo -e "${RED}Error: cyclictest not found${NC}"
    echo "Install with: sudo apt-get install rt-tests"
    exit 1
fi

# Check if running as root
if [ "$EUID" -ne 0 ]; then 
    echo -e "${YELLOW}Warning: Not running as root${NC}"
    echo "Some features may not be available (e.g., SCHED_FIFO)"
    echo "Run with sudo for full functionality"
    echo ""
fi

# Default parameters
DURATION=${1:-60}        # Test duration in seconds (default: 60s)
PRIORITY=${2:-80}        # RT priority (default: 80)
CPUS=${3:-""}           # CPU list (default: all CPUs)

echo "Test configuration:"
echo "  Duration: ${DURATION}s"
echo "  RT Priority: $PRIORITY"

if [ -n "$CPUS" ]; then
    echo "  CPUs: $CPUS"
    CPU_ARGS="-a $CPUS"
else
    NCPUS=$(nproc)
    echo "  CPUs: all ($NCPUS cores)"
    CPU_ARGS="-a"
fi

echo ""
echo "Starting cyclictest..."
echo "This will take $DURATION seconds. Press Ctrl+C to stop early."
echo ""

# Prepare output file
TIMESTAMP=$(date +%Y%m%d_%H%M%S)
OUTPUT_FILE="cyclictest_${TIMESTAMP}.log"

# Build cyclictest command
# -m: lock memory
# -n: use nanosleep
# -p: priority
# -t: number of threads (one per CPU)
# -l: number of loops
# -q: quiet mode
# -h: histogram output

LOOPS=$((DURATION * 1000000))  # Convert seconds to microseconds

cyclictest \
    -m \
    -n \
    -p "$PRIORITY" \
    $CPU_ARGS \
    -l "$LOOPS" \
    -h 100 \
    -q \
    | tee "$OUTPUT_FILE"

echo ""
echo -e "${GREEN}Test complete${NC}"
echo "Results saved to: $OUTPUT_FILE"
echo ""

# Parse and display summary
echo -e "${BLUE}=== Latency Summary ===${NC}"

# Extract max latencies
MAX_LATENCIES=$(grep "Max Latencies" "$OUTPUT_FILE" || echo "")
if [ -n "$MAX_LATENCIES" ]; then
    echo "$MAX_LATENCIES"
else
    # Try alternative format
    grep "T:" "$OUTPUT_FILE" | tail -1
fi

echo ""

# Interpret results
echo -e "${BLUE}=== Interpretation ===${NC}"
echo "For hard real-time applications (IEC 61850 GOOSE/SV):"
echo "  - Max latency <100 µs: Excellent"
echo "  - Max latency <200 µs: Good"
echo "  - Max latency <500 µs: Acceptable"
echo "  - Max latency >500 µs: Poor (investigate system)"
echo ""
echo "Tips to reduce latency:"
echo "  1. Use PREEMPT_RT kernel"
echo "  2. Isolate CPUs (isolcpus=2,3)"
echo "  3. Pin IRQs to non-isolated CPUs"
echo "  4. Disable GRO/LRO (./scripts/disable_gro_lro.sh)"
echo "  5. Set CPU governor to 'performance'"
echo "  6. Disable power management (C-states)"
echo "  7. Use cpuset in docker-compose.yml"
echo ""

# Check for concerning values
MAX_VALUE=$(grep -oP "Max:\s+\K\d+" "$OUTPUT_FILE" | sort -n | tail -1 || echo "0")
if [ "$MAX_VALUE" -gt 500 ]; then
    echo -e "${RED}⚠ Warning: High latency detected (${MAX_VALUE} µs)${NC}"
    echo "   This system may not be suitable for hard real-time applications"
    exit 1
elif [ "$MAX_VALUE" -gt 200 ]; then
    echo -e "${YELLOW}⚠ Caution: Moderate latency (${MAX_VALUE} µs)${NC}"
    echo "   System is functional but could be improved"
    exit 0
else
    echo -e "${GREEN}✓ Good latency performance (${MAX_VALUE} µs)${NC}"
    echo "   System is well-configured for real-time applications"
    exit 0
fi
