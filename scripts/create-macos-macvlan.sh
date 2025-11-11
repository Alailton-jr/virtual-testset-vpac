#!/bin/bash
# Create macvlan network for Docker on macOS
# This allows the container to have direct access to your LAN

set -e

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

echo -e "${BLUE}=== Virtual TestSet - macOS macvlan Network Setup ===${NC}"
echo ""

# Check if running on macOS
if [[ "$OSTYPE" != "darwin"* ]]; then
    echo -e "${RED}Error: This script is for macOS only${NC}"
    exit 1
fi

# Check if Docker is running
if ! docker info > /dev/null 2>&1; then
    echo -e "${RED}Error: Docker is not running${NC}"
    echo "Please start Docker Desktop and try again"
    exit 1
fi

# Detect network interface
echo -e "${YELLOW}Step 1: Detecting network interfaces...${NC}"
echo ""
echo "Available interfaces:"
ifconfig | grep "^[a-z]" | cut -d: -f1

echo ""
echo -e "${YELLOW}Common interfaces:${NC}"
echo "  - en0: WiFi (most common for laptops)"
echo "  - en1: Ethernet (Thunderbolt/USB adapter)"
echo "  - bridge100: Docker Desktop bridge"
echo ""

# Prompt for interface
read -p "Enter network interface to use (default: en0): " INTERFACE
INTERFACE=${INTERFACE:-en0}

# Verify interface exists
if ! ifconfig "$INTERFACE" > /dev/null 2>&1; then
    echo -e "${RED}Error: Interface $INTERFACE not found${NC}"
    exit 1
fi

echo -e "${GREEN}✓ Using interface: $INTERFACE${NC}"
echo ""

# Get current network configuration
echo -e "${YELLOW}Step 2: Detecting network configuration...${NC}"

# Get IP address and subnet
IP_INFO=$(ifconfig "$INTERFACE" | grep "inet " | awk '{print $2}')
if [ -z "$IP_INFO" ]; then
    echo -e "${RED}Error: No IP address found on $INTERFACE${NC}"
    echo "Make sure you're connected to a network"
    exit 1
fi

# Extract network details
CURRENT_IP="$IP_INFO"
NETMASK=$(ifconfig "$INTERFACE" | grep "inet " | awk '{print $4}')

# Convert netmask to CIDR (simplified for common cases)
case "$NETMASK" in
    0xffffff00) CIDR=24 ;;
    0xffff0000) CIDR=16 ;;
    0xffffff80) CIDR=25 ;;
    *) CIDR=24 ;; # default
esac

# Extract network prefix (assuming /24)
NETWORK_PREFIX=$(echo "$CURRENT_IP" | cut -d. -f1-3)
SUBNET="${NETWORK_PREFIX}.0/${CIDR}"
GATEWAY="${NETWORK_PREFIX}.1"

echo -e "${GREEN}✓ Detected network:${NC}"
echo "  Current IP: $CURRENT_IP"
echo "  Subnet: $SUBNET"
echo "  Gateway: $GATEWAY"
echo ""

# Prompt for confirmation or custom values
read -p "Use these settings? (y/n, default: y): " CONFIRM
CONFIRM=${CONFIRM:-y}

if [[ "$CONFIRM" != "y" && "$CONFIRM" != "Y" ]]; then
    echo ""
    read -p "Enter subnet (e.g., 192.168.1.0/24): " CUSTOM_SUBNET
    read -p "Enter gateway (e.g., 192.168.1.1): " CUSTOM_GATEWAY
    SUBNET=${CUSTOM_SUBNET:-$SUBNET}
    GATEWAY=${CUSTOM_GATEWAY:-$GATEWAY}
fi

# Determine IP range for containers (upper half of subnet)
IP_RANGE="${NETWORK_PREFIX}.128/25"

echo ""
echo -e "${YELLOW}Step 3: Creating macvlan network...${NC}"
echo ""
echo "Network configuration:"
echo "  Interface: $INTERFACE"
echo "  Subnet: $SUBNET"
echo "  Gateway: $GATEWAY"
echo "  IP Range: $IP_RANGE (for containers)"
echo ""

# Check if network already exists
if docker network inspect vts-macvlan > /dev/null 2>&1; then
    echo -e "${YELLOW}Warning: Network 'vts-macvlan' already exists${NC}"
    read -p "Remove and recreate? (y/n): " RECREATE
    if [[ "$RECREATE" == "y" || "$RECREATE" == "Y" ]]; then
        echo "Removing existing network..."
        docker network rm vts-macvlan
    else
        echo "Keeping existing network"
        exit 0
    fi
fi

# Create macvlan network
echo "Creating macvlan network..."
docker network create -d macvlan \
  --subnet="$SUBNET" \
  --gateway="$GATEWAY" \
  --ip-range="$IP_RANGE" \
  -o parent="$INTERFACE" \
  vts-macvlan

if [ $? -eq 0 ]; then
    echo -e "${GREEN}✓ macvlan network created successfully!${NC}"
else
    echo -e "${RED}✗ Failed to create network${NC}"
    exit 1
fi

echo ""
echo -e "${YELLOW}Step 4: Verifying network...${NC}"
docker network inspect vts-macvlan | grep -E "Name|Subnet|Gateway|IPRange|parent"

echo ""
echo -e "${GREEN}=== Setup Complete ===${NC}"
echo ""
echo "Next steps:"
echo "  1. Edit docker-compose.macos-macvlan.yml"
echo "  2. Update the container IP address (currently 192.168.1.200)"
echo "  3. Choose an unused IP in the range $IP_RANGE"
echo "  4. Run: docker-compose -f docker-compose.macos-macvlan.yml up"
echo ""
echo -e "${YELLOW}Important Notes:${NC}"
echo "  - The container will have its own IP on your LAN"
echo "  - Your macOS host CANNOT communicate directly with the container (macvlan limitation)"
echo "  - To access from macOS, use another device or create a separate bridge"
echo "  - Other devices on your LAN can access the container directly"
echo ""
echo -e "${BLUE}For easier setup (without macvlan), use docker-compose.macos.yml instead${NC}"
