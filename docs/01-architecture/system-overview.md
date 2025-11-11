# System Overview

## Introduction

Virtual TestSet is a comprehensive IEC 61850 test platform for protection relay testing and validation. It provides both hardware-in-the-loop (HIL) and software-based testing capabilities for Sampled Values (SV), GOOSE messaging, and various protection function tests.

## Architecture Overview

```
┌─────────────────────────────────────────────────────────────────┐
│                         Frontend (React)                         │
│  ┌──────────┐  ┌──────────┐  ┌──────────┐  ┌──────────┐       │
│  │Dashboard │  │ Streams  │  │  Tests   │  │   Logs   │       │
│  └────┬─────┘  └────┬─────┘  └────┬─────┘  └────┬─────┘       │
│       │             │             │             │               │
│       └─────────────┴─────────────┴─────────────┘               │
│                      HTTP REST API                              │
│                      WebSocket (real-time)                      │
└────────────────────────┬────────────────────────────────────────┘
                         │
┌────────────────────────┴────────────────────────────────────────┐
│                      Backend (C++)                              │
│  ┌──────────────┐  ┌──────────────┐  ┌──────────────┐         │
│  │  API Server  │  │ Test Engine  │  │ SV Publisher │         │
│  │  (Crow)      │  │              │  │              │         │
│  └──────┬───────┘  └──────┬───────┘  └──────┬───────┘         │
│         │                 │                 │                  │
│  ┌──────┴───────┬─────────┴────────┬────────┴───────┐         │
│  │   Protocols   │   Core Engine    │   I/O Layer    │         │
│  │ - IEC 61850   │ - Sequencer      │ - Raw Sockets  │         │
│  │ - GOOSE       │ - Test Functions │ - PCAP         │         │
│  │ - SV (9-2)    │ - Phasor Math    │ - File I/O     │         │
│  └───────────────┴──────────────────┴────────────────┘         │
└─────────────────────────────────────────────────────────────────┘
                         │
                         ▼
┌─────────────────────────────────────────────────────────────────┐
│                    Network Layer                                │
│  ┌──────────────────────────────────────────────────────────┐  │
│  │  Ethernet (IEC 61850-9-2 SV / GOOSE)                     │  │
│  │  - Multicast 01-0C-CD-04-00-00 to 01-0C-CD-04-01-FF      │  │
│  │  - VLAN tagging (optional)                                │  │
│  │  - 4000-8000 samples/second                               │  │
│  └──────────────────────────────────────────────────────────┘  │
└─────────────────────────────────────────────────────────────────┘
```

## Key Components

### Frontend
- **Technology:** React + TypeScript + Vite
- **UI Library:** shadcn/ui (Tailwind CSS)
- **State Management:** Zustand
- **API Client:** Axios + WebSocket

### Backend
- **Language:** C++17
- **Build System:** CMake
- **Web Framework:** Crow (HTTP/WebSocket)
- **Dependencies:**
  - libpcap (packet capture)
  - nlohmann/json (JSON parsing)
  - fftw3 (FFT for harmonics)

### Protocols
- **IEC 61850-9-2LE:** Sampled Values
- **IEC 61850-8-1:** GOOSE messaging
- **COMTRADE:** Waveform playback

## Communication Flow

### REST API
```
Frontend → HTTP POST /api/v1/streams → Backend
Frontend ← JSON Response            ← Backend
```

### WebSocket (Real-time)
```
Frontend → WS Connect /ws/logs     → Backend
Frontend ← Log Streaming           ← Backend
Frontend ← Analyzer Data           ← Backend
Frontend ← Test Progress           ← Backend
```

### Network Packets
```
Backend → Raw Socket → Ethernet → DUT (Device Under Test)
Backend ← PCAP       ← Ethernet ← DUT
```

## Deployment Models

### Docker Compose (Recommended)
```yaml
services:
  backend:
    - C++ server
    - Port 8080 (API)
    - Network capabilities (CAP_NET_RAW)
  
  frontend:
    - nginx + React build
    - Port 3000 (Web UI)
    - Proxies API to backend:8080
```

### Bare Metal (Linux)
- Backend runs as root or with CAP_NET_RAW
- Frontend served by nginx/Apache
- Direct network access for SV/GOOSE

### macOS/Windows (Limited)
- Backend runs in `--no-net` mode
- No raw socket access
- File-based testing (COMTRADE) only

## Data Flow Examples

### Stream Configuration
1. User configures stream in Frontend → `/streams`
2. Frontend sends `POST /api/v1/streams` with JSON config
3. Backend validates and stores configuration
4. Backend starts SV publisher thread
5. Packets transmitted on Ethernet

### Test Execution
1. User starts test in Frontend → `/overcurrent`
2. Frontend sends `POST /api/v1/tests/overcurrent`
3. Backend sequencer loads test points
4. For each point:
   - Update phasors
   - Transmit SV packets
   - Wait for trip signal or timeout
   - Record results
5. Backend returns results JSON
6. Frontend displays pass/fail table

### Real-time Monitoring
1. Frontend opens WebSocket → `/ws/analyzer`
2. Backend captures packets via libpcap
3. Backend decodes IEC 61850 frames
4. Backend streams JSON to WebSocket
5. Frontend updates analyzer table in real-time

## Security Considerations

- **Authentication:** Not implemented (assume trusted network)
- **HTTPS:** Not required (local deployment)
- **Network Isolation:** Recommended (dedicated test network)
- **Input Validation:** JSON schema validation on API endpoints

## Performance Characteristics

| Metric | Linux | Windows | macOS |
|--------|-------|---------|-------|
| SV Rate | 4000 sps | N/A | N/A |
| Latency | <10µs | N/A | N/A |
| Jitter | <5µs | N/A | N/A |
| API Throughput | 1000 req/s | 1000 req/s | 1000 req/s |

## Extensibility

### Adding New Protection Tests
1. Implement test logic in `backend/src/tests/`
2. Add API endpoint in `backend/src/api/`
3. Create frontend page in `frontend/src/pages/`
4. Update routing and navigation

### Adding New Protocols
1. Implement protocol encoder in `backend/src/protocols/`
2. Integrate with SV publisher or create new publisher
3. Add frontend configuration UI
4. Update API endpoints

## References

- [Backend Architecture](./backend-architecture.md)
- [Frontend Architecture](./frontend-architecture.md)
- [IEC 61850 Protocol Details](../03-backend/protocols.md)
- [API Reference](../03-backend/api-reference.md)
