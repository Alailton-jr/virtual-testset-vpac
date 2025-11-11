# Virtual TestSet - Backend (C/C++ Core Engine)

This directory contains the C/C++ core engine for the Virtual TestSet project.

## Contents

- `src/` - Source code for the core engine
  - `api/` - API implementations
  - `goose/` - GOOSE protocol implementation
  - `main/` - Main application entry point
  - `platform/` - Platform-specific code
  - `protocols/` - Protocol implementations
  - `sampledValue/` - Sampled value handling
  - `sniffer/` - Network sniffer implementation
  - `tools/` - Utility tools
- `tests/` - Unit tests
- `scripts/` - Build and runtime scripts
- `build/` - Build artifacts (generated)

## Building

See the following documentation for build instructions:
- [macOS Build Instructions](README-macos.md)
- [Docker Build Instructions](README_DOCKER.md)
- [Real-Time Configuration](README-RT.md)
- [Build Status](BUILD_STATUS.md)

## Quick Start

```bash
# Build on macOS
./scripts/build_macos.sh

# Or using CMake directly
cmake -B build -S .
cmake --build build

# Run tests
cd build && ctest

# Using Docker
docker-compose up
```

## Development

The project uses:
- CMake for build configuration
- Google Test for unit testing
- C++17 standard
- Platform-specific optimizations for real-time performance
