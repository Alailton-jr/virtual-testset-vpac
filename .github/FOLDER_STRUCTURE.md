# Repository Structure

This document describes the organization of the Virtual TestSet repository.

## Directory Layout

```
Virtual-TestSet/
├── backend/              # C/C++ Core Engine
│   ├── src/             # Source code
│   ├── tests/           # Unit tests
│   ├── scripts/         # Build and runtime scripts
│   ├── build/           # Build artifacts (generated)
│   ├── CMakeLists.txt   # CMake configuration
│   ├── Dockerfile       # Container image
│   └── README.md        # Backend documentation
│
├── frontend/            # (Coming soon) Web Control Interface
│   └── README.md        # Frontend documentation
│
├── .github/             # GitHub workflows and documentation
├── .vscode/             # VS Code workspace settings
├── LICENSE              # MIT License
└── README.md            # Main project documentation
```

## Components

### Backend (`backend/`)

The backend is a high-performance, real-time C/C++ application that implements:
- IEC 61850-9-2 Sampled Values (SV) protocol
- IEC 61850-8-1 GOOSE protocol
- Real-time packet generation with microsecond precision
- Network sniffing and protocol analysis

**Technologies:** C++17, CMake, Google Test, Docker

**Platforms:** Linux (production with RT kernel), macOS (development mode)

### Frontend (`frontend/`) - Coming Soon

The frontend will provide a web-based interface to:
- Configure test scenarios
- Start/stop test generators
- Monitor real-time metrics
- Visualize waveforms and protocol messages
- Manage multiple backend instances

**Planned Technologies:** React/Vue.js, TypeScript, WebSocket for real-time data

## Rationale

This structure separates concerns:
1. **Backend**: Low-level, real-time protocol handling
2. **Frontend**: User-facing control and visualization

This separation allows:
- Independent development and deployment
- Backend can run headless in production
- Frontend can connect to multiple backend instances
- Clear API boundaries between components
