#!/bin/bash
# Phase 9: Verify real-time environment configuration
# Checks kernel, CPU isolation, IRQ affinity, and Docker capabilities

set -e

# Color output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

PASS="${GREEN}✓${NC}"
FAIL="${RED}✗${NC}"
WARN="${YELLOW}⚠${NC}"

echo -e "${BLUE}=== Real-Time Environment Verification ===${NC}"
echo ""

# Track overall status
CRITICAL_FAILURES=0
WARNINGS=0

# 1. Kernel Check
echo -e "${BLUE}[1] Kernel Configuration${NC}"

# Check for PREEMPT_RT kernel
if uname -v | grep -q PREEMPT_RT; then
    echo -e "  $PASS RT kernel detected (PREEMPT_RT)"
    KERNEL_TYPE="PREEMPT_RT"
elif uname -v | grep -q PREEMPT; then
    echo -e "  $WARN Standard preemptible kernel (not RT-patched)"
    echo "      Recommendation: Use PREEMPT_RT kernel for hard real-time"
    WARNINGS=$((WARNINGS + 1))
    KERNEL_TYPE="PREEMPT"
else
    echo -e "  $FAIL No preemption support detected"
    CRITICAL_FAILURES=$((CRITICAL_FAILURES + 1))
    KERNEL_TYPE="NONE"
fi

echo "      Kernel: $(uname -r)"
echo ""

# 2. CPU Configuration
echo -e "${BLUE}[2] CPU Configuration${NC}"

# Count CPUs
NCPUS=$(nproc)
echo "  Total CPUs: $NCPUS"

# Check for isolated CPUs
ISOLATED_CPUS=$(cat /sys/devices/system/cpu/isolated 2>/dev/null || echo "")
if [ -n "$ISOLATED_CPUS" ]; then
    echo -e "  $PASS CPU isolation: $ISOLATED_CPUS"
else
    echo -e "  $WARN No CPUs isolated (isolcpus not set)"
    echo "      Add isolcpus=2,3 to kernel boot parameters"
    WARNINGS=$((WARNINGS + 1))
fi

# Check CPU governor
GOVERNOR=$(cat /sys/devices/system/cpu/cpu0/cpufreq/scaling_governor 2>/dev/null || echo "unknown")
if [ "$GOVERNOR" = "performance" ]; then
    echo -e "  $PASS CPU governor: performance"
else
    echo -e "  $WARN CPU governor: $GOVERNOR (recommend 'performance')"
    echo "      Set with: cpupower frequency-set -g performance"
    WARNINGS=$((WARNINGS + 1))
fi

echo ""

# 3. Memory Configuration
echo -e "${BLUE}[3] Memory Configuration${NC}"

# Check huge pages
HUGEPAGES=$(cat /proc/sys/vm/nr_hugepages 2>/dev/null || echo "0")
if [ "$HUGEPAGES" -gt 0 ]; then
    echo -e "  $PASS Huge pages configured: $HUGEPAGES"
else
    echo -e "  $WARN No huge pages configured"
    echo "      Set with: echo 128 > /proc/sys/vm/nr_hugepages"
    WARNINGS=$((WARNINGS + 1))
fi

# Check swappiness
SWAPPINESS=$(cat /proc/sys/vm/swappiness 2>/dev/null || echo "60")
if [ "$SWAPPINESS" -le 10 ]; then
    echo -e "  $PASS Low swappiness: $SWAPPINESS"
else
    echo -e "  $WARN High swappiness: $SWAPPINESS (recommend ≤10)"
    echo "      Set with: sysctl vm.swappiness=1"
    WARNINGS=$((WARNINGS + 1))
fi

echo ""

# 4. Network Configuration
echo -e "${BLUE}[4] Network Configuration${NC}"

# Check for network interfaces
INTERFACES=$(ip -br link show | grep -v "lo" | awk '{print $1}')
if [ -z "$INTERFACES" ]; then
    echo -e "  $FAIL No network interfaces found"
    CRITICAL_FAILURES=$((CRITICAL_FAILURES + 1))
else
    echo "  Network interfaces:"
    for iface in $INTERFACES; do
        # Check GRO/LRO status
        if ethtool -k "$iface" 2>/dev/null | grep -q "generic-receive-offload: on"; then
            echo -e "      $WARN $iface: GRO enabled (may increase latency)"
            echo "          Disable with: sudo ./scripts/disable_gro_lro.sh $iface"
            WARNINGS=$((WARNINGS + 1))
        else
            echo -e "      $PASS $iface: GRO disabled"
        fi
    done
fi

echo ""

# 5. Docker Configuration
echo -e "${BLUE}[5] Docker Configuration${NC}"

if ! command -v docker &> /dev/null; then
    echo -e "  $FAIL Docker not installed"
    CRITICAL_FAILURES=$((CRITICAL_FAILURES + 1))
else
    echo -e "  $PASS Docker installed: $(docker --version)"
    
    # Check if Docker daemon is running
    if docker info &> /dev/null; then
        echo -e "  $PASS Docker daemon running"
        
        # Check for Docker Compose
        if docker compose version &> /dev/null; then
            echo -e "  $PASS Docker Compose available: $(docker compose version --short)"
        else
            echo -e "  $WARN Docker Compose not available"
            WARNINGS=$((WARNINGS + 1))
        fi
    else
        echo -e "  $FAIL Docker daemon not running"
        CRITICAL_FAILURES=$((CRITICAL_FAILURES + 1))
    fi
fi

echo ""

# 6. Real-Time Capabilities
echo -e "${BLUE}[6] Real-Time Capabilities${NC}"

# Check if we can set real-time priority
if command -v chrt &> /dev/null; then
    echo -e "  $PASS chrt available (for RT scheduling)"
    
    # Check RT limits
    RT_PRIO_LIMIT=$(ulimit -r 2>/dev/null || echo "0")
    if [ "$RT_PRIO_LIMIT" = "unlimited" ] || [ "$RT_PRIO_LIMIT" -ge 95 ]; then
        echo -e "  $PASS RT priority limit: $RT_PRIO_LIMIT"
    else
        echo -e "  $WARN RT priority limit: $RT_PRIO_LIMIT (recommend ≥95)"
        echo "      Set in /etc/security/limits.conf:"
        echo "      @realtime soft rtprio 95"
        echo "      @realtime hard rtprio 95"
        WARNINGS=$((WARNINGS + 1))
    fi
else
    echo -e "  $WARN chrt not available"
    WARNINGS=$((WARNINGS + 1))
fi

# Check for cyclictest (latency testing)
if command -v cyclictest &> /dev/null; then
    echo -e "  $PASS cyclictest available (for latency testing)"
else
    echo -e "  $WARN cyclictest not available"
    echo "      Install: sudo apt-get install rt-tests"
    WARNINGS=$((WARNINGS + 1))
fi

echo ""

# 7. Build Tools
echo -e "${BLUE}[7] Build Tools${NC}"

# Check for required build tools
TOOLS=("cmake" "gcc" "g++" "make")
for tool in "${TOOLS[@]}"; do
    if command -v "$tool" &> /dev/null; then
        echo -e "  $PASS $tool available"
    else
        echo -e "  $FAIL $tool not found"
        CRITICAL_FAILURES=$((CRITICAL_FAILURES + 1))
    fi
done

echo ""

# Summary
echo -e "${BLUE}=== Summary ===${NC}"
echo ""

if [ $CRITICAL_FAILURES -eq 0 ] && [ $WARNINGS -eq 0 ]; then
    echo -e "${GREEN}✓ All checks passed${NC}"
    echo "  Environment is ready for real-time applications"
    exit 0
elif [ $CRITICAL_FAILURES -eq 0 ]; then
    echo -e "${YELLOW}⚠ $WARNINGS warning(s)${NC}"
    echo "  Environment is functional but not optimal"
    echo "  Review warnings above for performance improvements"
    exit 0
else
    echo -e "${RED}✗ $CRITICAL_FAILURES critical failure(s), $WARNINGS warning(s)${NC}"
    echo "  Environment is NOT ready for real-time applications"
    echo "  Fix critical issues above before running"
    exit 1
fi
