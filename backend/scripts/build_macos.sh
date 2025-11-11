#!/bin/bash
# ============================================================================
# Virtual TestSet - macOS Build Script (Phase 11)
# ============================================================================
# This script builds Virtual TestSet on macOS with platform-specific settings.
# Network operations are disabled by default on macOS (no AF_PACKET support).
#
# Usage:
#   ./scripts/build_macos.sh [clean]
#
# Options:
#   clean   Remove build directory before building
#
# Requirements:
#   - macOS 10.15+ (Catalina or later)
#   - Xcode Command Line Tools
#   - CMake 3.20+
#
# Features on macOS:
#   - RT functions are no-ops with INFO logging
#   - RawSocket is stubbed (no actual network I/O)
#   - Defaults to --no-net mode
#   - Self-test mode works without network
# ============================================================================

set -e  # Exit on error

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
echo -e "${BLUE}Virtual TestSet - macOS Build${NC}"
echo -e "${BLUE}========================================${NC}"

# Check if running on macOS
if [[ "$(uname)" != "Darwin" ]]; then
    echo -e "${RED}ERROR: This script is for macOS only${NC}"
    echo -e "${YELLOW}For Linux builds, use: cmake --preset linux-release${NC}"
    exit 1
fi

# Check for clean flag
if [[ "$1" == "clean" ]]; then
    echo -e "${YELLOW}Cleaning build directory...${NC}"
    rm -rf "$PROJECT_ROOT/build"
    echo -e "${GREEN}Build directory cleaned${NC}"
fi

# Check for CMake
if ! command -v cmake &> /dev/null; then
    echo -e "${RED}ERROR: CMake not found${NC}"
    echo -e "${YELLOW}Install with: brew install cmake${NC}"
    exit 1
fi

# Check for Xcode Command Line Tools
if ! xcode-select -p &> /dev/null; then
    echo -e "${RED}ERROR: Xcode Command Line Tools not found${NC}"
    echo -e "${YELLOW}Install with: xcode-select --install${NC}"
    exit 1
fi

echo -e "${GREEN}✓ macOS build environment OK${NC}"
echo ""

# Create build directory
cd "$PROJECT_ROOT"
mkdir -p build
cd build

echo -e "${BLUE}Configuring with CMake (macos-dev preset)...${NC}"
cmake .. --preset macos-dev || {
    echo -e "${YELLOW}Preset not found, using manual configuration...${NC}"
    cmake .. \
        -DCMAKE_BUILD_TYPE=Debug \
        -DCMAKE_CXX_STANDARD=17 \
        -DCMAKE_EXPORT_COMPILE_COMMANDS=ON \
        -DENABLE_ASAN=OFF \
        -DENABLE_TSAN=OFF \
        -DENABLE_UBSAN=OFF
}

echo ""
echo -e "${BLUE}Building...${NC}"
cmake --build . --config Debug -j$(sysctl -n hw.ncpu)

echo ""
echo -e "${GREEN}========================================${NC}"
echo -e "${GREEN}Build complete!${NC}"
echo -e "${GREEN}========================================${NC}"
echo ""
echo -e "${YELLOW}Binary location:${NC} $PROJECT_ROOT/build/Debug/virtual-testset"
echo ""
echo -e "${YELLOW}Run with:${NC}"
echo -e "  ./build/Debug/virtual-testset --help"
echo -e "  ./build/Debug/virtual-testset --selftest"
echo -e "  ./build/Debug/virtual-testset --no-net"
echo ""
echo -e "${YELLOW}Or use convenience script:${NC}"
echo -e "  ./scripts/run_macos_no_net.sh"
echo ""
echo -e "${BLUE}Note: Network operations disabled on macOS (no AF_PACKET)${NC}"
echo -e "${BLUE}RT functions are no-ops with INFO logging${NC}"
echo ""
