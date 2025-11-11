# Getting Started with Virtual TestSet Implementation

This guide helps you get started implementing the Virtual TestSet project based on the comprehensive integration plan.

## Prerequisites

### System Requirements

- **Operating System**: Linux (Ubuntu 22.04+), macOS (11.0+), or Windows (10/11 with Npcap)
- **CPU**: Multi-core processor (4+ cores recommended)
- **RAM**: 8GB minimum, 16GB recommended
- **Disk**: 20GB free space
- **Network**: Ethernet adapter for network testing (optional for development)

### Deployment Options

Virtual TestSet can be run in two ways:

| Method | Performance | Setup | Network I/O | Best For |
|--------|-------------|-------|-------------|----------|
| **Native Build** | ⭐⭐⭐⭐⭐ (10-200µs) | Medium | ✅ Full | Production, performance testing |
| **Docker** | ⭐⭐ (1-5ms) | Easy | ⚠️ Limited | Development, CI/CD |

**Recommendation:** Use **native builds** for 5-25x better performance and full network capabilities!

### Software Dependencies

#### For Native Builds (Recommended)

```bash
# Ubuntu/Debian
sudo apt-get update
sudo apt-get install -y \
    build-essential \
    cmake \
    git \
    libpcap-dev \
    libfftw3-dev \
    ninja-build \
    clang-format \
    clang-tidy \
    cppcheck

# macOS
brew install cmake ninja libpcap fftw
```

#### For Frontend Development

```bash
# Node.js 20+ (use nvm)
curl -o- https://raw.githubusercontent.com/nvm-sh/nvm/v0.39.0/install.sh | bash
nvm install 20
nvm use 20

# Verify
node --version  # Should be v20.x.x
npm --version   # Should be 10.x.x
```

#### For Docker Development

```bash
# Docker Engine 24.0+
# Follow: https://docs.docker.com/engine/install/

# Docker Compose v2
docker compose version  # Should be v2.20.0+
```

## Quick Start

### Choose Your Path

**Path A: Native Build (Best Performance)** ⭐ Recommended for production
- 5-25x faster than Docker (<10-200µs latency vs 1-5ms)
- Full network packet injection/capture
- Real hardware testing capabilities
- See [Native Build Setup Guide](../02-setup/native-build-guide.md)

**Path B: Docker (Easiest Setup)**
- No manual dependencies
- Consistent environment
- Good for development and CI/CD
- Instructions below

### Path A: Native Build Quick Start

#### Linux

```bash
# 1. Install dependencies
sudo apt-get install -y build-essential cmake git libpcap-dev libfftw3-dev nodejs npm

# 2. Clone repository
git clone https://github.com/Alailton-jr/Virtual-TestSet.git
cd Virtual-TestSet

# 3. Build backend
cd backend
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j$(nproc)

# 4. Build frontend
cd ../frontend
npm install
npm run dev  # Access at http://localhost:5173

# 5. Run backend (in another terminal)
cd ../backend/build
sudo ./Main  # or use: sudo setcap cap_net_raw+ep ./Main && ./Main
```

#### macOS

```bash
# 1. Install dependencies
brew install cmake ninja libpcap fftw node

# 2. Clone repository
git clone https://github.com/Alailton-jr/Virtual-TestSet.git
cd Virtual-TestSet

# 3. Build backend
cd backend
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build

# 4. Build frontend
cd ../frontend
npm install
npm run dev  # Access at http://localhost:5173

# 5. Run backend (in another terminal)
cd ../backend/build
sudo ./Main  # Requires sudo for BPF device access
```

#### Windows

```powershell
# 1. Install prerequisites:
#    - Visual Studio 2022 (with C++ workload)
#    - CMake 3.20+
#    - Npcap from https://npcap.com/ (in WinPcap API-compatible Mode)
#    - Node.js 18+

# 2. Clone repository
git clone https://github.com/Alailton-jr/Virtual-TestSet.git
cd Virtual-TestSet

# 3. Build backend (in "x64 Native Tools Command Prompt for VS 2022")
cd backend
cmake -S . -B build -G "Visual Studio 17 2022"
cmake --build build --config Release

# 4. Build frontend
cd ..\frontend
npm install
npm run dev  # Access at http://localhost:5173

# 5. Run backend (in another terminal, may need Administrator)
cd ..\backend\build\bin\Release
.\Main.exe
```

**For detailed native build instructions:** See [Native Build Setup Guide](../02-setup/native-build-guide.md)

---

### Path B: Docker Quick Start

### 1. Clone the Repository

```bash
git clone https://github.com/Alailton-jr/Virtual-TestSet.git
cd Virtual-TestSet
```

### 2. Review the Structure

The project is organized as follows:

```
Virtual-TestSet/
├── backend/           # C++ core engine
├── frontend/          # React web UI
├── docker/            # Docker configurations ✅ DONE
├── schemas/           # JSON API schemas ✅ DONE
├── docs/              # Documentation
└── tests/             # End-to-end tests
```

### 3. Set Up Third-Party Dependencies (Backend)

```bash
cd backend

# Create third_party directory
mkdir -p third_party

# Add dependencies as submodules
git submodule add https://github.com/yhirose/cpp-httplib third_party/cpp-httplib
git submodule add https://github.com/zaphoyd/websocketpp third_party/websocketpp
git submodule add https://github.com/nlohmann/json third_party/json
git submodule add https://github.com/chriskohlhoff/asio third_party/asio

# Initialize and update
git submodule init
git submodule update
```

### 4. Install Frontend Dependencies

```bash
cd frontend

# Install packages
npm install

# Install dev tools
npm install -D \
    tailwindcss \
    postcss \
    autoprefixer \
    @types/node

# Initialize Tailwind
npx tailwindcss init -p

# Install UI components
npm install \
    @radix-ui/react-dialog \
    @radix-ui/react-dropdown-menu \
    @radix-ui/react-select \
    @radix-ui/react-tabs \
    @radix-ui/react-slider \
    class-variance-authority \
    clsx \
    tailwind-merge \
    lucide-react \
    zustand \
    @tanstack/react-router \
    react-hook-form \
    zod \
    @hookform/resolvers \
    recharts

# Install dev dependencies
npm install -D \
    @testing-library/react \
    @testing-library/jest-dom \
    @testing-library/user-event \
    vitest \
    @vitest/ui \
    jsdom \
    msw \
    @playwright/test
```

### 5. Build with Docker (Recommended)

```bash
# From project root
cd docker

# Build and start (development profile)
docker compose --profile dev up --build

# Access the application
# Frontend: http://localhost:5173
# Backend API: http://localhost:8080
# Backend WS: ws://localhost:8090
```

### 6. Build Locally (Alternative)

#### Backend

```bash
cd backend

# Configure
cmake -B build -S . \
    -DCMAKE_BUILD_TYPE=Debug \
    -GNinja

# Build
cmake --build build -j$(nproc)

# Run tests
cd build && ctest --output-on-failure

# Run application
./build/Main --help
```

#### Frontend

```bash
cd frontend

# Development server
npm run dev

# Build for production
npm run build

# Preview production build
npm run preview
```

## Development Workflow

### Backend Development

1. **Create a new module**:
   ```bash
   mkdir -p backend/src/mymodule/{include,src}
   touch backend/src/mymodule/CMakeLists.txt
   ```

2. **Add to build**:
   Edit `backend/CMakeLists.txt` and add:
   ```cmake
   add_subdirectory(src/mymodule)
   ```

3. **Write tests**:
   ```bash
   touch backend/tests/unit/test_mymodule.cpp
   ```

4. **Format code**:
   ```bash
   find backend/src -name "*.cpp" -o -name "*.hpp" | xargs clang-format -i
   ```

5. **Run tests**:
   ```bash
   cd backend/build
   ctest --output-on-failure
   ```

### Frontend Development

1. **Create a new component**:
   ```bash
   mkdir -p frontend/src/components/mycomponent
   touch frontend/src/components/mycomponent/MyComponent.tsx
   ```

2. **Create a new page**:
   ```bash
   touch frontend/src/pages/MyPage.tsx
   ```

3. **Add tests**:
   ```bash
   touch frontend/src/components/mycomponent/__tests__/MyComponent.test.tsx
   ```

4. **Run tests**:
   ```bash
   cd frontend
   npm test
   ```

5. **Lint and format**:
   ```bash
   npm run lint
   npm run format
   ```

### Docker Development

1. **Rebuild specific service**:
   ```bash
   docker compose --profile dev build backend
   docker compose --profile dev up backend
   ```

2. **View logs**:
   ```bash
   docker compose --profile dev logs -f backend
   docker compose --profile dev logs -f frontend
   ```

3. **Execute commands in container**:
   ```bash
   docker compose exec backend bash
   docker compose exec frontend sh
   ```

4. **Clean up**:
   ```bash
   docker compose --profile dev down -v
   ```

## Implementation Priority

Follow this order for fastest value delivery:

### Phase 1: Foundation (Week 1-2)

- [ ] Set up HTTP/WS servers in backend
- [ ] Create basic REST endpoints (health, streams CRUD)
- [ ] Set up frontend shell with routing
- [ ] Create API client wrappers
- [ ] Verify end-to-end connectivity

### Phase 2: Core Features (Week 3-6)

- [ ] Implement SV Publisher Manager
- [ ] Implement Phasor Synthesizer
- [ ] Build SV Publisher Management UI (Module 13)
- [ ] Build Manual Injection UI (Module 2)
- [ ] Verify SV packet transmission

### Phase 3: Playback & Analysis (Week 7-10)

- [ ] Implement COMTRADE parser
- [ ] Build COMTRADE UI (Module 1)
- [ ] Implement Analyzer engine
- [ ] Build Analyzer UI (Module 5)
- [ ] Verify analysis accuracy

### Phase 4: Testing Modules (Week 11-16)

- [ ] Implement Sequence Engine
- [ ] Build Sequencer UI (Module 3)
- [ ] Implement GOOSE Subscriber
- [ ] Build GOOSE UI (Module 4)
- [ ] Implement Testers (Ramping, Distance, OC, Diff)
- [ ] Build Tester UIs (Modules 6-11)

### Phase 5: Polish (Week 17-20)

- [ ] Write comprehensive tests
- [ ] Complete documentation
- [ ] Set up CI/CD
- [ ] Performance optimization
- [ ] Security hardening

## Testing Strategy

### Unit Tests

```bash
# Backend (GoogleTest)
cd backend/build
ctest --output-on-failure

# Frontend (Vitest)
cd frontend
npm test
```

### Integration Tests

```bash
# Backend integration tests
cd backend/build
./vts_tests --gtest_filter=Integration*

# Frontend with MSW
cd frontend
npm run test:integration
```

### E2E Tests

```bash
# Start services
docker compose --profile dev up -d

# Run Playwright tests
cd frontend
npx playwright test

# With UI
npx playwright test --ui
```

## Debugging

### Backend

```bash
# With GDB
cd backend/build
gdb ./Main
(gdb) run --api-port 8080
(gdb) bt  # backtrace on crash

# With Valgrind
valgrind --leak-check=full ./Main --api-port 8080

# With sanitizers
cmake -B build -S . -DENABLE_ASAN=ON
cmake --build build
./build/Main --api-port 8080
```

### Frontend

```bash
# Browser DevTools
# Open Chrome DevTools (F12)
# Use React DevTools extension
# Use Redux DevTools for Zustand

# VS Code debugging
# Create .vscode/launch.json:
{
  "type": "chrome",
  "request": "launch",
  "name": "Debug Frontend",
  "url": "http://localhost:5173",
  "webRoot": "${workspaceFolder}/frontend/src"
}
```

### Docker

```bash
# Check logs
docker compose logs backend
docker compose logs frontend

# Execute shell
docker compose exec backend bash

# Inspect network
docker network inspect vts_vts_net

# Monitor resources
docker stats
```

## Common Issues

### Backend won't compile

```bash
# Ensure dependencies are installed
sudo apt-get install -y libpcap-dev libfftw3-dev

# Clean and rebuild
rm -rf backend/build
cmake -B backend/build -S backend
cmake --build backend/build
```

### Frontend won't start

```bash
# Clear node_modules and reinstall
cd frontend
rm -rf node_modules package-lock.json
npm install

# Clear cache
npm cache clean --force
```

### Docker build fails

```bash
# Clean Docker cache
docker system prune -af
docker volume prune -f

# Rebuild without cache
docker compose build --no-cache
```

### Port already in use

```bash
# Find process using port
lsof -i :8080
lsof -i :5173

# Kill process
kill -9 <PID>

# Or change port in docker-compose.yml
```

## Resources

- **Documentation**: [docs/ROADMAP.md](ROADMAP.md)
- **API Reference**: [docs/API.md](API.md) (to be created)
- **Architecture**: [docs/DECISIONS.md](DECISIONS.md) (to be created)
- **Integration Plan**: [integration-imple.md](../integration-imple.md)
- **IEC 61850 Standards**: https://www.iec.ch/
- **C++ Best Practices**: https://isocpp.github.io/CppCoreGuidelines/
- **React Best Practices**: https://react.dev/learn

## Getting Help

- **GitHub Issues**: https://github.com/Alailton-jr/Virtual-TestSet/issues
- **Discussions**: https://github.com/Alailton-jr/Virtual-TestSet/discussions

## Contributing

1. Fork the repository
2. Create a feature branch (`git checkout -b feature/my-feature`)
3. Commit your changes (`git commit -am 'Add my feature'`)
4. Push to the branch (`git push origin feature/my-feature`)
5. Create a Pull Request

## License

See [LICENSE](../LICENSE) file for details.
