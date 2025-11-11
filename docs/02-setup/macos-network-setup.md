# Running Virtual TestSet on macOS with Network Support

## Overview

This guide explains how to send SV/GOOSE packets from a **Linux container** running on **macOS** via Docker Desktop.

## The Challenge

- macOS doesn't support AF_PACKET raw sockets natively
- Docker Desktop on macOS runs containers in a Linux VM
- By default, the VM's network is isolated from the host's physical interfaces

## Solution: Bridge Networking via Docker Desktop

### Prerequisites

1. **Docker Desktop for Mac** (version 4.0+)
2. **macOS 11+** (Big Sur or later)
3. **Admin privileges** (for network configuration)

---

## Method 1: Docker Desktop with Host Network Access (Recommended)

### Step 1: Enable Experimental Features

Edit Docker Desktop settings:

```bash
# Open Docker Desktop preferences
# Navigate to: Docker Engine
# Add to the configuration:
{
  "experimental": true,
  "debug": true
}
```

### Step 2: Create Bridge Network

```bash
# Create a custom bridge network
docker network create \
  --driver bridge \
  --subnet 172.28.0.0/16 \
  --gateway 172.28.0.1 \
  --opt "com.docker.network.bridge.enable_ip_masquerade=true" \
  --opt "com.docker.network.bridge.name=vts-bridge" \
  vts-net

# Verify network creation
docker network inspect vts-net
```

### Step 3: Run Container with Network Access

```bash
# Run VTS container with bridge network
docker run -it --rm \
  --name vts \
  --network vts-net \
  --cap-add NET_ADMIN \
  --cap-add NET_RAW \
  --cap-add NET_BROADCAST \
  --device /dev/net/tun \
  -e IF_NAME=eth0 \
  -e ALLOW_MULTICAST=true \
  -p 8080:8080 \
  -p 8090:8090 \
  vts:latest
```

### Step 4: Configure Multicast Routing (on macOS host)

```bash
# Enable IP forwarding on macOS
sudo sysctl -w net.inet.ip.forwarding=1

# Add multicast route for IEC 61850 (if needed)
sudo route add -net 224.0.0.0/4 172.28.0.1
```

---

## Method 2: Docker Compose with macvlan (Advanced)

### Step 1: Identify Network Interface

```bash
# List network interfaces on macOS
ifconfig

# Typical interfaces:
# - en0: WiFi
# - en1: Ethernet (Thunderbolt)
# - bridge100: Docker Desktop bridge
```

### Step 2: Create macvlan Network

```bash
# Create macvlan network (replace en0 with your interface)
docker network create -d macvlan \
  --subnet=192.168.1.0/24 \
  --gateway=192.168.1.1 \
  --ip-range=192.168.1.128/25 \
  -o parent=en0 \
  vts-macvlan

# Note: This gives containers direct access to your LAN
```

### Step 3: Docker Compose Configuration

Create `docker-compose.macos.yml`:

```yaml
version: '3.8'

services:
  vts:
    image: vts:latest
    container_name: vts-macos
    networks:
      vts-macvlan:
        ipv4_address: 192.168.1.200  # Choose an unused IP
    
    cap_add:
      - NET_ADMIN
      - NET_RAW
      - NET_BROADCAST
    
    environment:
      - IF_NAME=eth0
      - ALLOW_MULTICAST=true
      - VTS_PLATFORM=LINUX  # Container is Linux
    
    ports:
      - "8080:8080"
      - "8090:8090"
    
    volumes:
      - ./backend/files:/app/files

networks:
  vts-macvlan:
    external: true
```

### Step 4: Run with Docker Compose

```bash
# Create the macvlan network first (if not done)
docker network create -d macvlan \
  --subnet=192.168.1.0/24 \
  --gateway=192.168.1.1 \
  --ip-range=192.168.1.128/25 \
  -o parent=en0 \
  vts-macvlan

# Start the service
docker-compose -f docker-compose.macos.yml up
```

---

## Method 3: socat Bridge (Packet Forwarding)

This method forwards packets between macOS and the container using `socat`.

### Step 1: Install socat on macOS

```bash
brew install socat
```

### Step 2: Run Container with Port Mapping

```bash
docker run -it --rm \
  --name vts \
  --cap-add NET_ADMIN \
  --cap-add NET_RAW \
  -p 8080:8080 \
  -p 8090:8090 \
  -p 10102:10102/udp \
  vts:latest
```

### Step 3: Create Packet Bridge Script

Create `macos-bridge.sh`:

```bash
#!/bin/bash
# Bridge between macOS and container for multicast packets

CONTAINER_IP=$(docker inspect -f '{{range .NetworkSettings.Networks}}{{.IPAddress}}{{end}}' vts)
MULTICAST_ADDR="224.0.0.1"  # IEC 61850 multicast
PORT=10102

echo "Bridging multicast traffic between macOS and container at $CONTAINER_IP"

# Forward multicast packets from macOS to container
socat UDP4-RECVFROM:$PORT,ip-add-membership=$MULTICAST_ADDR:en0,fork \
      UDP4-SENDTO:$CONTAINER_IP:$PORT &

# Forward container packets to macOS multicast
socat UDP4-RECVFROM:$PORT,bind=$CONTAINER_IP,fork \
      UDP4-SENDTO:$MULTICAST_ADDR:$PORT,ip-multicast-if=en0 &

echo "Bridge running. Press Ctrl+C to stop."
wait
```

```bash
chmod +x macos-bridge.sh
./macos-bridge.sh
```

---

## Method 4: Host Network Mode (Docker Desktop 4.29+)

Docker Desktop 4.29+ introduced host network support for macOS.

### Step 1: Update Docker Desktop

```bash
# Ensure you have Docker Desktop 4.29 or later
docker --version
```

### Step 2: Enable Host Networking

```bash
# Run with --network host (experimental on macOS)
docker run -it --rm \
  --network host \
  --cap-add NET_ADMIN \
  --cap-add NET_RAW \
  -e IF_NAME=en0 \
  vts:latest
```

**Note**: This is experimental and may not work reliably on all macOS versions.

---

## Testing Network Connectivity

### Inside Container

```bash
# Check interfaces
docker exec -it vts ip addr show

# Test multicast send
docker exec -it vts python3 -c "
import socket
sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
sock.setsockopt(socket.IPPROTO_IP, socket.IP_MULTICAST_TTL, 2)
sock.sendto(b'test', ('224.0.0.1', 10102))
print('Multicast packet sent')
"

# Check raw socket capability
docker exec -it vts python3 -c "
import socket
try:
    sock = socket.socket(socket.AF_PACKET, socket.SOCK_RAW)
    print('✅ Raw sockets available')
except:
    print('❌ Raw sockets not available')
"
```

### On macOS Host

```bash
# Listen for multicast packets
sudo tcpdump -i en0 -n 'net 224.0.0.0/4'

# Or use socat
socat UDP4-RECVFROM:10102,ip-add-membership=224.0.0.1:en0,fork STDOUT
```

---

## Recommended Configuration

For most use cases on macOS, use **Method 1 (Bridge Network)**:

```yaml
# docker-compose.macos-bridge.yml
version: '3.8'

services:
  vts:
    image: vts:latest
    container_name: vts-macos
    
    networks:
      - vts-net
    
    cap_add:
      - NET_ADMIN
      - NET_RAW
      - SYS_NICE      # For RT priorities
      - IPC_LOCK      # For memory locking
    
    ulimits:
      rtprio: 95
      memlock: -1
    
    environment:
      - IF_NAME=eth0
      - API_PORT=8080
      - WS_PORT=8090
      - ALLOW_MULTICAST=true
    
    ports:
      - "8080:8080"
      - "8090:8090"
    
    volumes:
      - ./backend/files:/app/files
    
    restart: unless-stopped

networks:
  vts-net:
    driver: bridge
    ipam:
      config:
        - subnet: 172.28.0.0/16
          gateway: 172.28.0.1
```

Run with:

```bash
docker-compose -f docker-compose.macos-bridge.yml up
```

---

## Limitations on macOS

Even with these methods, macOS networking has limitations:

1. **No AF_PACKET on macOS host** - The macOS binary can't use raw sockets
2. **Container is Linux** - Only works when running Linux container
3. **Performance** - Higher latency than native Linux (~1-5ms overhead)
4. **Multicast** - May require additional routing configuration
5. **Docker VM** - All traffic goes through Docker Desktop's VM

### Performance Expectations

| Platform | Latency | Jitter | Use Case |
|----------|---------|--------|----------|
| Native Linux | <10µs | <5µs | Production |
| Linux Container on Linux | <50µs | <10µs | Production |
| Linux Container on macOS | ~1-5ms | ~1-10ms | Development/Testing |
| macOS Native | N/A | N/A | No raw sockets |

---

## Troubleshooting

### Issue: "Cannot create raw socket"

```bash
# Ensure capabilities are granted
docker run --cap-add NET_ADMIN --cap-add NET_RAW ...

# Check inside container
docker exec -it vts capsh --print | grep cap_net_raw
```

### Issue: "Multicast packets not reaching container"

```bash
# Enable IP forwarding on macOS
sudo sysctl -w net.inet.ip.forwarding=1

# Add multicast route
sudo route add -net 224.0.0.0/4 172.28.0.1

# Check Docker network driver
docker network inspect vts-net | grep Driver
```

### Issue: "Interface not found"

```bash
# List interfaces inside container
docker exec -it vts ip link show

# The container interface is typically 'eth0', not 'en0'
# Update environment variable:
-e IF_NAME=eth0
```

### Issue: "Permission denied" for raw sockets

```bash
# Run container with additional privileges
docker run --privileged ...

# Or specific capabilities
--cap-add NET_ADMIN --cap-add NET_RAW --cap-add NET_BROADCAST
```

---

## Production Recommendation

For **production IEC 61850 systems**, we strongly recommend:

1. ✅ **Native Linux server** - Best performance and reliability
2. ✅ **Linux VM on macOS** (VMware Fusion, Parallels) - With bridged networking
3. ⚠️ **Docker on macOS** (this guide) - Development/testing only
4. ❌ **Native macOS** - Not supported for packet transmission

---

## Summary

You can send SV/GOOSE packets from a **Linux container** on **macOS** using:

1. **Bridge networking** (easiest, recommended)
2. **macvlan networking** (direct LAN access)
3. **socat bridge** (packet forwarding)
4. **Host mode** (experimental, Docker Desktop 4.29+)

The container runs **Linux** (with raw socket support), but traffic is routed through Docker Desktop's VM, adding some latency.

For **microsecond-level timing**, use native Linux. For **development and testing**, these methods work well on macOS.
