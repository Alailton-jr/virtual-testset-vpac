# Docker Deployment for Virtual TestSet

This directory contains Docker configurations for deploying the Virtual TestSet application.

## Quick Start

### Development Mode (Bridge Network)

```bash
# Build and start services
docker compose --profile dev up --build

# Access the application at http://localhost:5173
```

### Real-Time Mode (Host Network)

For production deployments requiring real-time performance and raw socket access:

```bash
# Start with RT profile
docker compose --profile rt up --build

# Application accessible at http://localhost (port 80)
```

## Services

### Backend (`vts-backend`)

- **Base Image**: Ubuntu 24.04
- **Capabilities**: NET_ADMIN, NET_RAW, SYS_NICE (RT mode also includes IPC_LOCK)
- **Ports**: 
  - 8080 (REST API)
  - 8090 (WebSocket)
- **Health Check**: TCP connection to port 8080

### Frontend (`vts-frontend`)

- **Base Image**: nginx:alpine
- **Port**: 5173 (dev) or 80 (rt)
- **Proxies**:
  - `/api` → backend:8080
  - `/ws` → backend:8090

## Profiles

| Profile | Network Mode | Use Case |
|---------|--------------|----------|
| `dev` | Bridge | Development, testing |
| `rt` | Host | Production, real-time performance |

## Environment Variables

Configure via `.env` file or shell environment:

```bash
IF_NAME=eth0          # Network interface for packet capture
API_PORT=8080         # REST API port (host network only)
WS_PORT=8090          # WebSocket port (host network only)
RT_PRIORITY=80        # Real-time thread priority (rt profile)
```

## Volumes

- `backend_data`: Persistent storage for COMTRADE files, logs, and uploads

## Network Configuration

### Development Mode
- Bridge network: `172.25.0.0/16`
- Backend: `172.25.0.10`
- Frontend: `172.25.0.20`

### Real-Time Mode
- Host network for minimum latency
- Direct access to physical interfaces

## Building

```bash
# Build individual services
docker compose build backend
docker compose build frontend

# Build with specific target
docker compose build --no-cache backend
```

## Running Tests

```bash
# Run backend unit tests
docker compose run --rm backend /usr/local/bin/vts_tests

# Run with specific test filter
docker compose run --rm backend /usr/local/bin/vts_tests --gtest_filter=ComtradeTest.*
```

## Troubleshooting

### Backend won't start
- Check network interface name: `docker compose exec backend ip addr`
- Verify capabilities: `docker compose exec backend capsh --print`

### Frontend can't reach backend
- Check backend health: `docker compose ps`
- Test API: `curl http://localhost:8080/api/v1/health`

### Permission denied on network operations
- Ensure capabilities are set correctly
- For RT mode, may need to run with elevated privileges

## Security Considerations

### Real-Time Mode
- Host network provides direct access to all network interfaces
- Elevated capabilities (NET_ADMIN, NET_RAW) allow packet injection
- Only deploy in trusted environments
- Consider firewall rules to limit exposure

### Development Mode
- Bridge network isolates services
- Limited capability set
- Suitable for untrusted environments

## Performance Tuning

For real-time performance:

1. **CPU Isolation**: Reserve CPUs for real-time threads
   ```bash
   # Add to kernel parameters
   isolcpus=2,3 nohz_full=2,3 rcu_nocbs=2,3
   ```

2. **IRQ Affinity**: Pin network IRQs to non-RT cores
   ```bash
   ./backend/scripts/pin_irqs.sh eth0 0,1
   ```

3. **Disable Power Saving**:
   ```bash
   cpupower frequency-set -g performance
   ```

4. **Huge Pages**: For memory lock efficiency
   ```bash
   echo 512 > /sys/kernel/mm/hugepages/hugepages-2048kB/nr_hugepages
   ```

See `backend/README-RT.md` for detailed real-time configuration.
