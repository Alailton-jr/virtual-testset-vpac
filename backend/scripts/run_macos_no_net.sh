#!/bin/bash
# ============================================================================
# Virtual TestSet - macOS No-Net Runner (Phase 11)
# ============================================================================
# This script runs Virtual TestSet on macOS in no-net mode.
# Perfect for testing configuration, module instantiation, and API access
# without requiring network hardware or privileges.
#
# Usage:
#   ./scripts/run_macos_no_net.sh [OPTIONS]
#
# Options:
#   --selftest    Run self-test and exit
#   --help        Show application help
#
# What works in no-net mode:
#   ✓ Configuration loading and validation
#   ✓ Module instantiation (Ethernet, GOOSE, SV)
#   ✓ TCP API server (port 8080)
#   ✓ Real-time utilities (no-op with INFO logs)
#   ✓ Self-test mode
#
# What doesn't work:
#   ✗ Raw socket operations (AF_PACKET not available)
#   ✗ Packet transmission
#   ✗ Network sniffing
#   ✗ Hardware timestamping
# ============================================================================

set -e

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# Script directory
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "$SCRIPT_DIR/.." && pwd)"

echo -e "${BLUE}========================================${NC}"
echo -e "${BLUE}Virtual TestSet - macOS No-Net Mode${NC}"
echo -e "${BLUE}========================================${NC}"

# Check if binary exists
BINARY="$PROJECT_ROOT/build/Debug/virtual-testset"
if [[ ! -f "$BINARY" ]]; then
    echo -e "${RED}ERROR: Binary not found at $BINARY${NC}"
    echo -e "${YELLOW}Build first with: ./scripts/build_macos.sh${NC}"
    exit 1
fi

echo -e "${GREEN}✓ Binary found${NC}"
echo ""

# Set environment variables
export VTS_NO_NET=1
export IF_NAME="en0"  # Dummy interface name (won't be used)

echo -e "${YELLOW}Environment:${NC}"
echo -e "  VTS_NO_NET = 1"
echo -e "  IF_NAME = en0 (dummy)"
echo ""

# Parse options
ARGS="--no-net"
for arg in "$@"; do
    case "$arg" in
        --selftest)
            ARGS="$ARGS --selftest"
            echo -e "${BLUE}Running in self-test mode...${NC}"
            ;;
        --help)
            ARGS="$ARGS --help"
            ;;
        *)
            echo -e "${YELLOW}Warning: Unknown option: $arg${NC}"
            ;;
    esac
done

echo -e "${GREEN}Starting Virtual TestSet (no-net mode)...${NC}"
echo ""
echo -e "${BLUE}========================================${NC}"
echo ""

# Run the application
"$BINARY" $ARGS

EXIT_CODE=$?

echo ""
echo -e "${BLUE}========================================${NC}"

if [[ $EXIT_CODE -eq 0 ]]; then
    echo -e "${GREEN}Application exited successfully${NC}"
else
    echo -e "${RED}Application exited with code $EXIT_CODE${NC}"
fi

echo -e "${BLUE}========================================${NC}"

exit $EXIT_CODE
