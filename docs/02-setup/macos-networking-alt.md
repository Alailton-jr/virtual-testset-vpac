# Running Virtual TestSet on macOS with Network Support 🍎

## Quick Start

Yes, you **can** send SV/GOOSE packets from a Linux container on macOS! Here's the easiest way:

```bash
# 1. Build the Linux image
cd /Users/alailtonjr/Github/Virtual-TestSet
docker build -f docker/Dockerfile.backend -t vts:latest .

# 2. Run with bridge networking (easiest method)
docker-compose -f docker/docker-compose.macos.yml up
```

Your container is now running **Linux** (with full raw socket support) on macOS! 🎉

---

## How It Works

### The Problem
- ❌ **macOS native**: No AF_PACKET raw sockets
- ❌ **macOS Docker**: Default isolation prevents network access

### The Solution
- ✅ **Linux container**: Has full AF_PACKET support
- ✅ **Bridge networking**: Routes packets through Docker Desktop's VM
- ✅ **Network capabilities**: Container has NET_RAW, NET_ADMIN

### Architecture

```
┌─────────────────────────────────────────────────┐
│  macOS Host (Your Mac)                          │
│                                                  │
│  ┌────────────────────────────────────────┐    │
│  │  Docker Desktop VM (Linux)             │    │
│  │                                          │    │
│  │  ┌──────────────────────────────────┐  │    │
│  │  │  VTS Container (Linux)           │  │    │
│  │  │                                   │  │    │
│  │  │  ✅ AF_PACKET support            │  │    │
│  │  │  ✅ Raw sockets                  │  │    │
│  │  │  ✅ SV/GOOSE transmission        │  │    │
│  │  └──────────────────────────────────┘  │    │
│  │           ↕ Bridge Network             │    │
│  └────────────────────────────────────────┘    │
│                ↕ NAT / Routing                  │
│         (en0 / en1 interface)                   │
└─────────────────────────────────────────────────┘
          ↕ Physical Network
    (LAN / IEC 61850 Network)
```

---

## Method 1: Bridge Network (Recommended) 🌉

### Setup

```bash
# Start with Docker Compose
docker-compose -f docker/docker-compose.macos.yml up

# Or manually
docker run -it --rm \
  --name vts \
  --network bridge \
  --cap-add NET_ADMIN \
  --cap-add NET_RAW \
  -p 8080:8080 \
  -p 8090:8090 \
  -e IF_NAME=eth0 \
  vts:latest
```

### Features
- ✅ Easy setup (no network configuration)
- ✅ Works on WiFi and Ethernet
- ✅ Containers can communicate
- ⚠️ Adds ~1-5ms latency (Docker VM overhead)

---

## Method 2: macvlan Network (Advanced) 🔗

Gives your container direct access to your LAN (appears as separate device).

### Setup

```bash
# 1. Run automated setup script
./scripts/create-macos-macvlan.sh

# 2. Start container
docker-compose -f docker/docker-compose.macos-macvlan.yml up
```

### Features
- ✅ Container has own IP on LAN
- ✅ Direct multicast access
- ✅ Can communicate with other IEDs
- ⚠️ More complex setup
- ⚠️ macOS host can't reach container directly

### Manual Setup

```bash
# Create macvlan network
docker network create -d macvlan \
  --subnet=192.168.1.0/24 \
  --gateway=192.168.1.1 \
  --ip-range=192.168.1.128/25 \
  -o parent=en0 \
  vts-macvlan

# Run container with static IP
docker run -it --rm \
  --name vts \
  --network vts-macvlan \
  --ip 192.168.1.200 \
  --cap-add NET_ADMIN \
  --cap-add NET_RAW \
  vts:latest
```

---

## Testing Network Functionality

### 1. Verify Raw Socket Support

```bash
# Check inside container
docker exec -it vts-macos python3 << 'EOF'
import socket
try:
    sock = socket.socket(socket.AF_PACKET, socket.SOCK_RAW)
    print("✅ Raw sockets available!")
    print(f"   Container is running: Linux")
except Exception as e:
    print(f"❌ Raw sockets not available: {e}")
EOF
```

### 2. Check Network Interfaces

```bash
# List interfaces inside container
docker exec -it vts-macos ip addr show

# Should show:
# - lo: loopback
# - eth0: container interface (bridge or macvlan)
```

### 3. Test Multicast

```bash
# Send multicast packet from container
docker exec -it vts-macos python3 << 'EOF'
import socket
import struct

MCAST_GRP = '224.0.0.1'
MCAST_PORT = 10102

sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM, socket.IPPROTO_UDP)
sock.setsockopt(socket.IPPROTO_IP, socket.IP_MULTICAST_TTL, 2)

message = b'Test multicast from VTS container'
sock.sendto(message, (MCAST_GRP, MCAST_PORT))
print(f"✅ Sent multicast to {MCAST_GRP}:{MCAST_PORT}")
EOF

# Listen on macOS host
sudo tcpdump -i en0 -n 'net 224.0.0.0/4'
```

### 4. Test SV Transmission

```bash
# Send via API
curl -X POST http://localhost:8080/api/stream/start \
  -H "Content-Type: application/json" \
  -d '{
    "stream_id": "test-sv",
    "interface": "eth0",
    "dst_mac": "01:0c:cd:01:00:01"
  }'

# Check logs
docker logs vts-macos
```

---

## Performance Comparison

| Method | Latency | Jitter | Complexity | Use Case |
|--------|---------|--------|------------|----------|
| Native Linux | <10µs | <5µs | ⭐ | Production |
| Linux on Linux | <50µs | <10µs | ⭐⭐ | Production |
| **Bridge on macOS** | ~1-5ms | ~1-10ms | ⭐ | **Development** |
| **macvlan on macOS** | ~1-5ms | ~1-10ms | ⭐⭐⭐ | Testing/Lab |
| Native macOS | N/A | N/A | N/A | Not supported |

---

## Troubleshooting

### "Cannot create raw socket"

```bash
# Ensure capabilities are granted
docker run --cap-add NET_ADMIN --cap-add NET_RAW ...

# Verify inside container
docker exec -it vts-macos capsh --print | grep cap_net_raw
```

### "Network unreachable" or "No route to host"

```bash
# Check Docker network exists
docker network ls | grep vts

# Inspect network configuration
docker network inspect vts-net

# Verify container has correct interface
docker exec -it vts-macos ip route show
```

### "Multicast packets not working"

```bash
# Enable IP forwarding on macOS
sudo sysctl -w net.inet.ip.forwarding=1

# Add multicast route
sudo route add -net 224.0.0.0/4 172.28.0.1

# Check multicast membership inside container
docker exec -it vts-macos ip maddr show
```

### "Cannot access container from macOS" (macvlan)

This is **expected behavior** with macvlan! Solutions:

1. **Use bridge network instead** (recommended)
2. **Access from another device** on the LAN
3. **Create secondary interface** on macOS:
   ```bash
   sudo ifconfig en0 alias 192.168.1.201
   ```

---

## Configuration Files

### Bridge Network (Easy)
- **File**: `docker/docker-compose.macos.yml`
- **Usage**: `docker-compose -f docker/docker-compose.macos.yml up`
- **Network**: 172.28.0.0/16 (bridge)
- **Access**: `http://localhost:8080`

### macvlan Network (Advanced)
- **File**: `docker/docker-compose.macos-macvlan.yml`
- **Setup**: `./scripts/create-macos-macvlan.sh`
- **Network**: Your LAN subnet (e.g., 192.168.1.0/24)
- **Access**: `http://192.168.1.200:8080` (from LAN)

---

## When to Use What

### Use **Bridge Network** if you want:
- ✅ Easy setup
- ✅ Access from macOS (localhost)
- ✅ Multiple containers
- ✅ Development and testing

### Use **macvlan Network** if you need:
- ✅ Container as separate device on LAN
- ✅ Direct IED communication
- ✅ Multicast group membership
- ✅ Lab/testing environment

### Use **Native Linux** if you need:
- ✅ Production deployment
- ✅ Microsecond timing (<10µs)
- ✅ Maximum performance
- ✅ RT kernel optimizations

---

## Summary

✅ **Yes, you can send SV packets from macOS!**

The container runs **Linux** (with full AF_PACKET support) inside Docker Desktop's VM. Network traffic is routed through the VM to your physical network interface.

**For development on macOS**: Use bridge networking (`docker-compose.macos.yml`)  
**For testing/lab on macOS**: Use macvlan networking (`docker-compose.macos-macvlan.yml`)  
**For production**: Deploy to native Linux server

Expected performance on macOS: **1-5ms latency** (suitable for testing, not production)

---

## Next Steps

1. **Build and run**:
   ```bash
   docker-compose -f docker/docker-compose.macos.yml up
   ```

2. **Test API**:
   ```bash
   curl http://localhost:8080/api/status
   ```

3. **Send test packet**:
   ```bash
   curl -X POST http://localhost:8080/api/stream/start -d '{"stream_id":"test"}'
   ```

4. **Check logs**:
   ```bash
   docker logs vts-macos -f
   ```

5. **For advanced setup**, see: `docs/MACOS_NETWORK_SETUP.md`

Enjoy IEC 61850 testing on your Mac! 🚀
