# Backend Architecture

## Overview

The backend is a C++17 application that implements IEC 61850 protocol support, test execution engines, and a REST/WebSocket API server.

## Technology Stack

- **Language:** C++17
- **Build System:** CMake 3.20+
- **Web Framework:** Crow (HTTP + WebSocket)
- **JSON:** nlohmann/json
- **Packet I/O:** libpcap
- **Math:** fftw3 (FFT for harmonics)
- **Testing:** Google Test

## Directory Structure

```
backend/
├── src/
│   ├── main/           # Entry point
│   ├── api/            # REST API endpoints
│   ├── core/           # Core engines (sequencer, test runner)
│   ├── protocols/      # IEC 61850, GOOSE, SV encoders
│   ├── sampledValue/   # SV publisher implementation
│   ├── io/             # File I/O, COMTRADE parser
│   ├── analyzer/       # Packet analyzer
│   ├── platform/       # Cross-platform abstractions
│   └── goose/          # GOOSE implementation
├── tests/              # Unit tests
├── third_party/        # Dependencies
└── CMakeLists.txt      # Build configuration
```

## Key Components

### 1. API Server (`src/api/`)

**Crow-based HTTP server:**
- REST endpoints at `/api/v1/*`
- WebSocket endpoints at `/ws/*`
- JSON request/response
- CORS enabled for frontend

**Key Endpoints:**
- `GET/POST /api/v1/streams` - Stream management
- `POST /api/v1/tests/*` - Test execution
- `GET /api/v1/status` - System status
- `WS /ws/logs` - Real-time log streaming
- `WS /ws/analyzer` - Packet analysis stream

### 2. SV Publisher (`src/sampledValue/`)

**Sampled Values (IEC 61850-9-2LE) generator:**
- Configurable sample rate (80-256 samples/cycle at 50/60Hz)
- 3-phase voltage and current generation
- Phasor-to-sample conversion
- Quality flags (good/invalid/test/questionable)
- VLAN tagging support
- Raw socket transmission (Linux only)

**Features:**
- Real-time phasor updates via API
- Harmonic injection
- Frequency deviation
- Synchronized sampling

### 3. GOOSE Publisher/Subscriber (`src/goose/`)

**GOOSE messaging:**
- Generic Object Oriented Substation Event protocol
- Boolean, integer, float data types
- State change detection
- Configurable retransmission
- Dataset management

### 4. Test Engines (`src/core/`)

**Protection function test implementations:**
- **Overcurrent (50/51):** ANSI/IEC curves, pickup/timing tests
- **Distance (21):** R-X impedance plane, fault angle sweep
- **Differential (87):** Operating/restraint characteristics
- **Ramping:** Variable ramping with KPI measurement

**Sequencer:**
- Multi-step test execution
- Timing control (dwell time, delays)
- Conditional branching
- Result logging

### 5. Analyzer (`src/analyzer/`)

**Real-time packet capture and decode:**
- libpcap-based capture
- IEC 61850 frame parsing
- GOOSE/SV identification
- Statistics (packet rate, errors)
- WebSocket streaming to frontend

### 6. COMTRADE Parser (`src/io/`)

**Waveform file support:**
- IEEE C37.111 COMTRADE format
- CSV waveform files
- Playback with timing control
- Sample interpolation

### 7. Platform Abstraction (`src/platform/`)

**Cross-platform support:**
- Real-time scheduling (Linux SCHED_FIFO, Windows priorities)
- CPU affinity
- Memory locking
- Network capabilities detection
- See [Cross-Platform Guide](../05-cross-platform/overview.md)

## Build Process

```bash
# Configure
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release

# Build
cmake --build build -j$(nproc)

# Run
./build/Main

# Run with options
./build/Main --no-net  # Disable network I/O (macOS/Windows)
```

## Threading Model

- **Main Thread:** API server (Crow)
- **SV Publisher Thread:** High-priority packet transmission
- **GOOSE Thread:** Event-driven messaging
- **Analyzer Thread:** Packet capture and processing
- **Test Thread:** Test execution (spawned per test)

## Memory Management

- RAII for resource management
- Smart pointers (`std::unique_ptr`, `std::shared_ptr`)
- No manual `new`/`delete`
- Stack allocation preferred

## Error Handling

- Exceptions for API errors (caught by Crow)
- Status codes in test results
- Logging via stdout (captured by frontend)
- Graceful degradation on unsupported platforms

## Performance

### Linux (Full Performance)
- **Packet Rate:** 4000 packets/second sustained
- **Latency:** <10µs from API call to packet transmission
- **Jitter:** <5µs (99th percentile) with RT kernel
- **CPU:** Single core at ~30% utilization

### Windows/macOS (Limited)
- No network I/O (`--no-net` mode required)
- File-based testing only (COMTRADE playback)
- API server fully functional

## Dependencies

| Library | Purpose | Version |
|---------|---------|---------|
| Crow | HTTP/WebSocket server | Latest |
| nlohmann/json | JSON parsing | 3.11+ |
| libpcap | Packet capture | 1.10+ |
| fftw3 | FFT (harmonics) | 3.3+ |
| GoogleTest | Unit testing | 1.14+ |

## Configuration

- **Port:** 8080 (configurable via `--port`)
- **Network Interface:** Auto-detect or `--interface eth0`
- **Log Level:** `--log-level debug|info|warn|error`

## API Documentation

See [API Reference](./api-reference.md) for detailed endpoint documentation.

## Testing

```bash
# Run unit tests
cd build
ctest --output-on-failure

# Run specific test
./tests/vts_tests --gtest_filter=SampledValueTest.*
```

## References

- [System Overview](./system-overview.md)
- [Cross-Platform Support](../05-cross-platform/overview.md)
- [COMTRADE Parser](../03-backend/comtrade-parser.md)
