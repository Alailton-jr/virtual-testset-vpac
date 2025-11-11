#!/bin/bash
# Phase 9: Pin network interface IRQs to specific CPU cores
# This isolates interrupt handling from real-time application CPUs

set -e

# Color output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

echo -e "${GREEN}=== Pinning IRQs for Real-Time Performance ===${NC}"
echo ""

# Check if running as root
if [ "$EUID" -ne 0 ]; then 
    echo -e "${RED}Error: This script must be run as root${NC}"
    echo "Usage: sudo $0 <interface> <cpu_list>"
    echo "Example: sudo $0 eth0 0,1"
    exit 1
fi

# Get arguments
INTERFACE=${1:-eth0}
CPU_LIST=${2:-0,1}

# Check if interface exists
if ! ip link show "$INTERFACE" &> /dev/null; then
    echo -e "${RED}Error: Interface $INTERFACE does not exist${NC}"
    echo "Available interfaces:"
    ip -br link show
    exit 1
fi

echo "Target interface: $INTERFACE"
echo "CPU list for IRQ pinning: $CPU_LIST"
echo ""

# Find IRQs for the interface
IRQS=$(grep "$INTERFACE" /proc/interrupts | awk '{print $1}' | sed 's/:$//')

if [ -z "$IRQS" ]; then
    echo -e "${YELLOW}Warning: No IRQs found for $INTERFACE${NC}"
    echo "This may be a virtual interface or IRQs are named differently."
    echo ""
    echo "Searching for alternative IRQ names..."
    
    # Try alternative names (e.g., eth0-TxRx-0)
    IRQS=$(grep -i "$(echo $INTERFACE | sed 's/[0-9]*$//')" /proc/interrupts | awk '{print $1}' | sed 's/:$//')
    
    if [ -z "$IRQS" ]; then
        echo -e "${RED}Error: Could not find any IRQs for $INTERFACE${NC}"
        echo ""
        echo "Available IRQs:"
        cat /proc/interrupts | head -20
        exit 1
    fi
fi

echo "Found IRQs:"
echo "$IRQS"
echo ""

# Convert CPU list to bitmask
# Example: "0,1" -> 0x3 (binary: 11)
#          "2,3" -> 0xC (binary: 1100)
calculate_cpu_mask() {
    local cpu_list=$1
    local mask=0
    
    IFS=',' read -ra CPUS <<< "$cpu_list"
    for cpu in "${CPUS[@]}"; do
        mask=$((mask | (1 << cpu)))
    done
    
    printf "%x" $mask
}

CPU_MASK=$(calculate_cpu_mask "$CPU_LIST")
echo "CPU mask: 0x$CPU_MASK"
echo ""

# Pin each IRQ to the specified CPUs
echo "Pinning IRQs..."
for irq in $IRQS; do
    SMP_AFFINITY_FILE="/proc/irq/$irq/smp_affinity"
    
    if [ ! -f "$SMP_AFFINITY_FILE" ]; then
        echo -e "${YELLOW}Warning: $SMP_AFFINITY_FILE not found, skipping IRQ $irq${NC}"
        continue
    fi
    
    echo -n "  IRQ $irq -> CPUs $CPU_LIST... "
    
    # Read current affinity
    OLD_AFFINITY=$(cat "$SMP_AFFINITY_FILE")
    
    # Set new affinity
    if echo "$CPU_MASK" > "$SMP_AFFINITY_FILE" 2>/dev/null; then
        echo -e "${GREEN}OK${NC} (was: 0x$OLD_AFFINITY)"
    else
        echo -e "${RED}Failed${NC}"
    fi
done

echo ""
echo "Current IRQ affinity:"
for irq in $IRQS; do
    if [ -f "/proc/irq/$irq/smp_affinity" ]; then
        AFFINITY=$(cat /proc/irq/$irq/smp_affinity)
        echo "  IRQ $irq: 0x$AFFINITY"
    fi
done

echo ""
echo -e "${GREEN}IRQ pinning complete${NC}"
echo ""
echo "Note: These settings are NOT persistent across reboots."
echo "To make permanent, add to /etc/rc.local or create a systemd service."
echo ""
echo "Recommendation: Pin IRQs to CPUs 0-1, run real-time app on CPUs 2-3"
echo "  - Use isolcpus=2,3 in kernel boot parameters"
echo "  - Set cpuset_cpus=\"2,3\" in docker-compose.yml"
echo "  - Use RT_CPU_AFFINITY=\"2,3\" environment variable"
