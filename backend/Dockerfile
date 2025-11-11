# Phase 9: Multi-stage Dockerfile for Real-Time Virtual Test Set
# This creates a minimal, RT-capable container for IEC 61850 testing

# ============================================================================
# Build Stage: Compile the application with all dependencies
# ============================================================================
FROM ubuntu:22.04 AS builder

# Install build dependencies
RUN apt-get update && apt-get install -y \
    build-essential \
    cmake \
    git \
    pkg-config \
    libfftw3-dev \
    nlohmann-json3-dev \
    && rm -rf /var/lib/apt/lists/*

# Set working directory
WORKDIR /build

# Copy source code
COPY . /build/

# Build the application
RUN mkdir -p build && cd build && \
    cmake .. \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_CXX_FLAGS="-O3 -march=native -mtune=native" && \
    make -j$(nproc)

# ============================================================================
# Runtime Stage: Minimal image with only runtime dependencies
# ============================================================================
FROM ubuntu:22.04

# Install runtime dependencies only
RUN apt-get update && apt-get install -y \
    libfftw3-3 \
    libstdc++6 \
    iproute2 \
    iputils-ping \
    tcpdump \
    ethtool \
    util-linux \
    && rm -rf /var/lib/apt/lists/*

# Create non-root user for running the application
# Note: Will run with elevated capabilities via docker-compose
RUN groupadd -r vts && useradd -r -g vts -s /bin/bash vts

# Create necessary directories
RUN mkdir -p /app /app/config /app/files /app/logs && \
    chown -R vts:vts /app

# Copy built binary from build stage
COPY --from=builder /build/build/virtual_testset /app/virtual_testset
RUN chmod +x /app/virtual_testset

# Copy configuration files if they exist (optional)
# Note: Add your JSON configs to /app/config/ via volume mount in docker-compose

# Set working directory
WORKDIR /app

# Switch to non-root user (capabilities will be added by docker-compose)
USER vts

# Environment variables for configuration
ENV IF_NAME=eth0
ENV RT_PRIORITY=80
ENV RT_CPU_AFFINITY=""

# Expose TCP port for API (if applicable)
EXPOSE 8080

# Health check to verify the application is responsive
HEALTHCHECK --interval=30s --timeout=5s --start-period=10s --retries=3 \
    CMD pgrep -f virtual_testset || exit 1

# Default command
CMD ["/app/virtual_testset"]

# ============================================================================
# Build instructions:
# docker build -t virtual-testset:latest .
#
# Run with docker-compose for proper RT capabilities:
# docker-compose up
#
# Manual run (testing only - missing RT capabilities):
# docker run --rm -it --network=host virtual-testset:latest
# ============================================================================
