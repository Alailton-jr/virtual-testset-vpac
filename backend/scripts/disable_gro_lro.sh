#!/bin/bash
# Phase 9: Disable GRO (Generic Receive Offload) and LRO (Large Receive Offload)
# These features can increase latency and jitter for real-time applications

set -e

# Color output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

echo -e "${GREEN}=== Disabling GRO/LRO for Real-Time Performance ===${NC}"
echo ""

# Check if running as root
if [ "$EUID" -ne 0 ]; then 
    echo -e "${RED}Error: This script must be run as root${NC}"
    echo "Usage: sudo $0 <interface>"
    exit 1
fi

# Get interface name (default to eth0 if not provided)
INTERFACE=${1:-eth0}

# Check if interface exists
if ! ip link show "$INTERFACE" &> /dev/null; then
    echo -e "${RED}Error: Interface $INTERFACE does not exist${NC}"
    echo "Available interfaces:"
    ip -br link show
    exit 1
fi

echo "Target interface: $INTERFACE"
echo ""

# Display current settings
echo "Current offload settings:"
ethtool -k "$INTERFACE" | grep -E "rx-gro|rx-lro|tx-gso|tx-gro"
echo ""

# Disable GRO (Generic Receive Offload)
echo -n "Disabling GRO... "
if ethtool -K "$INTERFACE" gro off 2>/dev/null; then
    echo -e "${GREEN}OK${NC}"
else
    echo -e "${YELLOW}Failed or not supported${NC}"
fi

# Disable LRO (Large Receive Offload)
echo -n "Disabling LRO... "
if ethtool -K "$INTERFACE" lro off 2>/dev/null; then
    echo -e "${GREEN}OK${NC}"
else
    echo -e "${YELLOW}Failed or not supported${NC}"
fi

# Disable GSO (Generic Segmentation Offload) - optional, may reduce throughput
echo -n "Disabling GSO (optional)... "
if ethtool -K "$INTERFACE" gso off 2>/dev/null; then
    echo -e "${GREEN}OK${NC}"
else
    echo -e "${YELLOW}Failed or not supported${NC}"
fi

# Disable TSO (TCP Segmentation Offload) - optional
echo -n "Disabling TSO (optional)... "
if ethtool -K "$INTERFACE" tso off 2>/dev/null; then
    echo -e "${GREEN}OK${NC}"
else
    echo -e "${YELLOW}Failed or not supported${NC}"
fi

echo ""
echo "Updated offload settings:"
ethtool -k "$INTERFACE" | grep -E "rx-gro|rx-lro|tx-gso|tx-tso"
echo ""

echo -e "${GREEN}GRO/LRO disabled successfully${NC}"
echo ""
echo "Note: These settings are NOT persistent across reboots."
echo "To make permanent, add to /etc/network/interfaces or create a systemd service."
echo ""
echo "Example systemd service:"
echo "  [Unit]"
echo "  Description=Disable GRO/LRO for real-time"
echo "  After=network.target"
echo ""
echo "  [Service]"
echo "  Type=oneshot"
echo "  ExecStart=$0 $INTERFACE"
echo "  RemainAfterExit=yes"
echo ""
echo "  [Install]"
echo "  WantedBy=multi-user.target"
