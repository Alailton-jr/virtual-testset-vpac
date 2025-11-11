# Running Virtual TestSet on macOS

## 🎯 Overview

Virtual TestSet can run on macOS in two modes:

1. **Native Build with BPF** (Recommended for network testing)
   - Requires sudo privileges
   - Full network packet capture and transmission
   - Best performance and functionality
   - Requires macOS build from source

2. **Docker Mode** (Good for development)
   - No sudo required
   - Limited network functionality (--no-net mode)
   - Easier setup
   - Good for UI development and non-network features

---

## 🔥 Native macOS Build with BPF (Full Network Support)

### Prerequisites

- macOS 11.0 (Big Sur) or later
- Xcode Command Line Tools
- CMake 3.20+
- Homebrew (for dependencies)

### Install Dependencies

```bash
brew install cmake nlohmann-json crow
```

### Build

```bash
cd backend
mkdir build && cd build
cmake ..
make -j$(sysctl -n hw.ncpu)
```

### Run with Network Support

**Important:** BPF requires root privileges (sudo) to access `/dev/bpf*` devices.

```bash
# Run backend with sudo for network access
sudo ./bin/Main

# In another terminal, run tests
sudo ./bin/vts_tests
```

### Available Network Interfaces

To list available network interfaces:

```bash
networksetup -listallhardwareports
# or
ifconfig | grep -E "^[a-z]" | cut -d: -f1
```

Common interfaces on macOS:
- `en0` - Primary Ethernet/Wi-Fi
- `en1` - Secondary network adapter
- `bridge0` - Virtual bridge interface

### Limitations

- **Requires sudo**: BPF devices (`/dev/bpf*`) require root access
- **Interface selection**: Backend automatically selects first available interface (typically `en0`)
- **Firewall**: May need to allow the application through macOS firewall

### Security Notes

Running with sudo gives the application full network access. Only run code you trust.

---

## 🚀 Docker Mode (Limited Network)

This mode runs backend in `--no-net` mode, suitable for development without network I/O.

### Quick Start

### Start Both Frontend and Backend

```bash
# From project root
docker compose -f docker/docker-compose.macos.yml up -d --build
```

### Access the Applications

- **Frontend UI**: http://localhost:5173
- **Backend API**: http://localhost:8080
- **Backend WebSocket**: http://localhost:8090

### Stop Containers

```bash
docker compose -f docker/docker-compose.macos.yml down
```

## 📋 Container Status

Check if containers are running:

```bash
docker ps | grep vts
```

View container logs:

```bash
# Backend logs
docker logs vts-backend-macos

# Frontend logs
docker logs vts-frontend-macos

# Follow logs in real-time
docker logs -f vts-backend-macos
```

## 🔧 Common Commands

### Restart Containers

```bash
docker compose -f docker/docker-compose.macos.yml restart
```

### Rebuild After Code Changes

```bash
# Rebuild backend only
docker compose -f docker/docker-compose.macos.yml up -d --build backend

# Rebuild frontend only
docker compose -f docker/docker-compose.macos.yml up -d --build frontend

# Rebuild both
docker compose -f docker/docker-compose.macos.yml up -d --build
```

### View Container Status

```bash
docker compose -f docker/docker-compose.macos.yml ps
```

### Execute Commands in Container

```bash
# Run backend tests
docker exec vts-backend-macos /usr/local/bin/vts_tests

# Access backend shell
docker exec -it vts-backend-macos bash

# Access frontend shell
docker exec -it vts-frontend-macos sh
```

## 🐛 Troubleshooting

### Backend Shows "Unhealthy"

This is **normal on macOS**. The backend runs with `--no-net` flag because:
- macOS Docker doesn't support raw packet interfaces like Linux
- The backend can't bind to network interfaces in the same way
- The healthcheck fails but the container is still functional for development

To ignore health status, the frontend doesn't depend on backend health.

### Port Already in Use

If you see "port is already allocated":

```bash
# Stop all VTS containers
docker stop $(docker ps -q --filter name=vts)

# Remove them
docker rm $(docker ps -aq --filter name=vts)

# Try starting again
docker compose -f docker/docker-compose.macos.yml up -d --build
```

### Frontend Not Accessible

Check logs:
```bash
docker logs vts-frontend-macos
```

Verify nginx is running:
```bash
docker exec vts-frontend-macos ps aux | grep nginx
```

### Backend API Not Responding

The backend runs with limited networking on macOS. For full functionality:
- Use the frontend UI which proxies requests
- Or deploy to Linux for production

## 🏗️ Architecture

### Network Configuration

- **Network**: `vts-net` (bridge network)
- **Subnet**: `172.25.0.0/16`
- **Backend IP**: `172.25.0.10`
- **Frontend IP**: `172.25.0.20`

### Volumes

- `backend_data`: Persists backend data
- `../backend/files`: Host directory mounted to container

### Port Mappings

| Service | Container Port | Host Port | Description |
|---------|---------------|-----------|-------------|
| Backend API | 8081 | 8080 | HTTP REST API |
| Backend WS | 8082 | 8090 | WebSocket |
| Frontend | 80 | 5173 | Web UI (Nginx) |

## 📝 Configuration Files

### Main Files

- `docker/docker-compose.macos.yml` - macOS-specific compose file
- `docker/Dockerfile.backend` - Backend build configuration
- `docker/Dockerfile.frontend` - Frontend build configuration
- `docker/nginx.conf` - Nginx web server config

### Environment Variables

Backend environment variables (in `docker-compose.macos.yml`):

```yaml
environment:
  - IF_NAME=eth0          # Container network interface
  - API_PORT=8081         # API port inside container
  - WS_PORT=8082          # WebSocket port inside container
  - VTS_PLATFORM=LINUX    # Platform identifier
  - RT_PRIORITY=80        # Real-time thread priority
```

## 🧪 Running Tests

### Frontend Tests (from host)

```bash
cd frontend
npm test
```

### Backend Tests (in container)

```bash
docker exec vts-backend-macos /usr/local/bin/vts_tests
```

### Full Test Suite

```bash
# Frontend unit tests
cd frontend && npm test

# Backend C++ tests
docker exec vts-backend-macos /usr/local/bin/vts_tests
```

## 🔄 Development Workflow

1. **Start containers**:
   ```bash
   docker compose -f docker/docker-compose.macos.yml up -d
   ```

2. **Make code changes**

3. **Rebuild specific service**:
   ```bash
   # Backend changes
   docker compose -f docker/docker-compose.macos.yml up -d --build backend
   
   # Frontend changes (or use local dev server)
   cd frontend && npm run dev  # Faster for development
   ```

4. **View logs**:
   ```bash
   docker logs -f vts-backend-macos
   ```

5. **Stop when done**:
   ```bash
   docker compose -f docker/docker-compose.macos.yml down
   ```

## 🚀 Production Deployment

**Note**: For production deployment with full networking capabilities:
- Use Linux host (Ubuntu 20.04+ or similar)
- Use `docker/docker-compose.yml` with `--profile dev` or `--profile rt`
- Ensure proper network interface configuration
- Enable real-time kernel for SRIOV profile

## 📊 Monitoring

### View Resource Usage

```bash
docker stats vts-backend-macos vts-frontend-macos
```

### Check Network

```bash
docker network inspect docker_vts-net
```

### View Volumes

```bash
docker volume ls | grep vts
docker volume inspect docker_backend_data
```

## ❓ FAQ

**Q: Why is the backend unhealthy?**  
A: On macOS, the backend runs with `--no-net` flag which disables full network functionality. This is a limitation of Docker on macOS.

**Q: Can I use this for production?**  
A: For production, deploy on Linux with full network support. This macOS setup is for development only.

**Q: How do I update to latest code?**  
A: Pull latest changes and rebuild: `git pull && docker compose -f docker/docker-compose.macos.yml up -d --build`

**Q: Can I run the frontend locally instead?**  
A: Yes! For faster development: `cd frontend && npm run dev` (port 5173)

**Q: Where are the container logs stored?**  
A: Docker manages logs. View with `docker logs <container_name>` or in Docker Desktop UI.

## 📚 Additional Resources

- [Docker Compose Documentation](https://docs.docker.com/compose/)
- [Docker Desktop for Mac](https://docs.docker.com/desktop/mac/)
- Project README: `../README.md`
- Frontend Tests: `UNIT_TESTS_SUMMARY.md`

---

**Last Updated**: November 8, 2025
