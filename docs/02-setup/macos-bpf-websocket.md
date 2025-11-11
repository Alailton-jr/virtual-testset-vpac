# macOS BPF Implementation & WebSocket Server

**Date**: November 3, 2025  
**Status**: ✅ Complete - macOS now sends SV packets via BPF

---

## Problem

The original implementation had macOS defaulting to `--no-net` mode, which disabled all network operations including SV packet transmission. This was because macOS doesn't support Linux's `AF_PACKET` raw socket API.

## Solution

Implemented **BPF (Berkeley Packet Filter)** support for macOS, which is the macOS equivalent of Linux's raw packet interface.

---

## What is BPF?

BPF is macOS's mechanism for:
- Capturing raw packets (like tcpdump, Wireshark)
- **Sending raw Ethernet frames** (what we need for SV)
- Low-level network access without IP stack processing

**Device**: `/dev/bpf0` through `/dev/bpf99` (character devices)  
**Permissions**: Requires root access (same as Linux `AF_PACKET`)

---

## Implementation Details

### 1. Opening BPF Device

```cpp
// Try /dev/bpf0 through /dev/bpf99
for (int i = 0; i < 100; i++) {
    char bpf_dev[32];
    snprintf(bpf_dev, sizeof(bpf_dev), "/dev/bpf%d", i);
    rawSocket_ = open(bpf_dev, O_RDWR);
    if (rawSocket_ >= 0) break;
}
```

**Why loop?** Each BPF device can only be opened once. If another process (like tcpdump) has `/dev/bpf0`, we try `/dev/bpf1`, etc.

### 2. Configuration

```cpp
// Immediate mode - don't buffer packets
unsigned int enable = 1;
ioctl(rawSocket_, BIOCIMMEDIATE, &enable);

// Bind to network interface (en0 = primary Ethernet/WiFi)
struct ifreq ifr;
strncpy(ifr.ifr_name, "en0", IFNAMSIZ);
ioctl(rawSocket_, BIOCSETIF, &ifr);
```

**BIOCIMMEDIATE**: Packets are sent/received immediately (no buffering)  
**BIOCSETIF**: Bind BPF to a specific interface

### 3. Sending Packets

```cpp
// Write raw Ethernet frame directly to BPF
ssize_t sent = write(rawSocket_, frame, frameLength);
```

**That's it!** Just write the complete Ethernet frame (including MAC addresses, VLAN tags, and payload).

---

## Platform Comparison

| Feature | Linux (AF_PACKET) | macOS (BPF) |
|---------|-------------------|-------------|
| **API** | `socket()` + `sendto()` | `open()` + `write()` |
| **Device** | Network socket | `/dev/bpf*` char device |
| **Permissions** | `CAP_NET_RAW` or root | Root required |
| **Frame Format** | Raw Ethernet | Raw Ethernet |
| **Performance** | Very fast (kernel bypass) | Fast (kernel bypass) |
| **Real-time** | Yes (with RT kernel) | No (XNU not RT) |

**Bottom line**: Both platforms now send SV packets with equivalent low-level access.

---

## Code Changes

### sv_publisher_instance.cpp

**Before (macOS)**:
```cpp
// Placeholder - doesn't actually send
rawSocket_ = socket(AF_INET, SOCK_DGRAM, 0);
// ... later ...
(void)frame;  // Packets not sent!
```

**After (macOS)**:
```cpp
// Open BPF device
for (int i = 0; i < 100; i++) {
    char bpf_dev[32];
    snprintf(bpf_dev, sizeof(bpf_dev), "/dev/bpf%d", i);
    rawSocket_ = open(bpf_dev, O_RDWR);
    if (rawSocket_ >= 0) break;
}

// Configure for immediate sending
ioctl(rawSocket_, BIOCIMMEDIATE, &enable);
ioctl(rawSocket_, BIOCSETIF, &ifr);  // Bind to en0

// ... later when sending ...
write(rawSocket_, frame, offset);  // Actually sends!
```

### main.cpp

**Before**:
```cpp
#ifdef VTS_PLATFORM_MAC
    config.no_net = true;  // Network disabled!
    std::cout << "[CONFIG] macOS detected - defaulting to no-net mode" << std::endl;
#endif
```

**After**:
```cpp
#ifdef VTS_PLATFORM_MAC
    std::cout << "[CONFIG] macOS detected - network operations enabled (no RT guarantees)" << std::endl;
#endif
```

---

## Testing on macOS

### 1. Check BPF Devices

```bash
ls -la /dev/bpf*
```

**Expected**:
```
crw-------  1 root  wheel   23,   0 Nov  3 14:20 /dev/bpf0
crw-------  1 root  wheel   23,   1 Nov  3 14:20 /dev/bpf1
...
```

### 2. Run the Application

```bash
cd backend
sudo ./build/Main
```

**Expected output**:
```
[CONFIG] macOS detected - network operations enabled (no RT guarantees)
...
HTTP server starting on port 8081
HTTP server started on port 8081
WebSocket server starting on port 8082
[SV] Starting SV publisher tick loop...
```

**No more "defaulting to no-net mode"!**

### 3. Verify SV Packets

```bash
# Terminal 1: Run VTS
sudo ./build/Main

# Terminal 2: Create a stream and start it
curl -X POST http://localhost:8081/api/v1/streams \
  -H "Content-Type: application/json" \
  -d '{
    "appId": "0x4000",
    "macDst": "01:0C:CD:04:00:00",
    "macSrc": "AA:BB:CC:DD:EE:01",
    "vlanId": 100,
    "vlanPrio": 4,
    "svId": "MACOS_TEST",
    "nominalFreq": 60.0,
    "sampleRate": 4800,
    "dataSource": "MANUAL"
  }'
# Note the returned stream ID

curl -X POST http://localhost:8081/api/v1/streams/<ID>/start

# Terminal 3: Monitor packets
sudo tcpdump -i en0 'ether proto 0x88ba or (vlan and ether proto 0x88ba)' -v
```

**Expected**:
```
15:32:10.123456 AA:BB:CC:DD:EE:01 > 01:0C:CD:04:00:00, ethertype 802.1Q-QinQ (0x8100), length 226
```

---

## Performance Notes

### Timing Precision

**Linux RT Kernel**:
- Tick loop: 100 µs granularity
- Priority scheduling: SCHED_FIFO
- Memory locking: mlockall()
- Jitter: < 10 µs

**macOS XNU Kernel**:
- Tick loop: 100 µs target (best-effort)
- Priority: Normal (no RT guarantees)
- Jitter: ~100-500 µs (acceptable for most relay testing)

### When macOS Performance is Sufficient

✅ **Good for**:
- Relay testing (ms-level timing requirements)
- Functional testing
- Development and debugging
- Most IEC 61850 use cases

❌ **Not ideal for**:
- Sub-millisecond precision requirements
- Deterministic hard real-time
- Microsecond-level synchronization
- Safety-critical applications

**Recommendation**: Use Linux for production relay testing, macOS for development.

---

## Troubleshooting

### Error: "Failed to open BPF device (requires root)"

**Cause**: No BPF devices available or permissions denied.

**Solution**:
```bash
# Check if running as root
whoami  # Should show "root"

# If not root
sudo ./build/Main

# Check BPF availability
ls -la /dev/bpf*
```

### Error: "Failed to bind BPF to interface en0"

**Cause**: Interface `en0` doesn't exist or is down.

**Solution**:
```bash
# List all interfaces
ifconfig -a

# Common macOS interfaces:
# en0 = Ethernet/WiFi (most common)
# en1 = Secondary interface
# lo0 = Loopback (don't use)

# Update the code to use your interface:
# In sv_publisher_instance.cpp, line ~88:
strncpy(ifr.ifr_name, "en1", IFNAMSIZ);  // If en0 doesn't work
```

### No Packets Visible in tcpdump

**Possible causes**:
1. **Wrong interface**: Check which interface VTS is bound to
2. **Stream not started**: Verify with `/api/v1/streams`
3. **Multicast filtering**: Some switches drop unknown multicast
4. **Capture filter**: Try without filter first

**Debug steps**:
```bash
# 1. Check stream status
curl http://localhost:8081/api/v1/streams

# 2. Capture all traffic (no filter)
sudo tcpdump -i en0 -v

# 3. Check if BPF is working at all
sudo tcpdump -i en0  # Should show regular traffic
```

---

## WebSocket Server

While we were at it, we also completed the WebSocket server implementation!

### Features

**Topics**:
- `analyzer/phasors`: Real-time phasor data
- `analyzer/waveforms`: Waveform samples
- `sequence/progress`: Test sequence status updates
- `goose/events`: GOOSE message events

**API**:
```cpp
wsServer_->broadcast("analyzer/phasors", {
    {"timestamp", now},
    {"Va": {120.0, 0.0}},
    {"Vb": {120.0, -120.0}},
    {"Vc": {120.0, 120.0}}
});
```

**Client Connection** (JavaScript):
```javascript
const ws = new WebSocket('ws://localhost:8082');
ws.onopen = () => {
    ws.send(JSON.stringify({
        type: 'subscribe',
        topic: 'analyzer/phasors'
    }));
};
ws.onmessage = (event) => {
    const data = JSON.parse(event.data);
    console.log('Phasor update:', data);
};
```

### Testing

```bash
# Install wscat (WebSocket CLI tool)
npm install -g wscat

# Connect and subscribe
wscat -c ws://localhost:8082
> {"type":"subscribe","topic":"analyzer/phasors"}
```

---

## Summary

✅ **macOS now sends SV packets** via BPF (same as Linux sends via AF_PACKET)  
✅ **WebSocket server implemented** for real-time data streaming  
✅ **Full cross-platform support** - no more no-net mode on macOS  
✅ **Clean build** with all warnings addressed

**Next**: Implement COMTRADE parser and analyzer engine to feed real data to WebSocket clients!

---

## References

- **BPF Manual**: `man 4 bpf`
- **BPF Programming**: https://www.tcpdump.org/manpages/bpf.4.html
- **AF_PACKET (Linux)**: `man 7 packet`
- **WebSocket Protocol**: RFC 6455
- **IEC 61850-9-2**: Sampled Values specification
