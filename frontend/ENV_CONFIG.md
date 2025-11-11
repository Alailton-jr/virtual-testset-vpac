# Frontend Environment Configuration

## Backend URL Configuration

The frontend can connect to backends running on different machines by configuring environment variables.

### Quick Start (Local Development)

For local development with both frontend and backend on the same machine, the default configuration works out of the box:

```bash
cd frontend
npm install
npm run dev
```

The frontend will automatically proxy to:
- API: `http://localhost:8081`
- WebSocket: `ws://localhost:8082`

### Remote Backend Configuration

When the backend is running on a different machine, create a `.env` file:

```bash
cd frontend
cp .env.example .env
```

Edit `.env` with your backend's address:

```bash
# For a backend at 192.168.1.100
VITE_API_URL=http://192.168.1.100:8081
VITE_WS_URL=ws://192.168.1.100:8082
```

Then restart the dev server:

```bash
npm run dev
```

### Configuration Options

#### Development (Same Machine)
```bash
VITE_API_URL=http://localhost:8081
VITE_WS_URL=ws://localhost:8082
```

#### Remote Development (Different Machine)
```bash
VITE_API_URL=http://192.168.1.100:8081
VITE_WS_URL=ws://192.168.1.100:8082
```

#### Docker Deployment
```bash
VITE_API_URL=http://backend:8081
VITE_WS_URL=ws://backend:8082
```

#### Production (HTTPS)
```bash
VITE_API_URL=https://api.your-domain.com
VITE_WS_URL=wss://ws.your-domain.com
```

### How It Works

1. **Development Mode** (npm run dev):
   - Vite's dev server proxies requests to the backend
   - Frontend makes requests to `/api/v1/...`
   - Vite forwards them to the configured backend URL
   - CORS is handled automatically by the proxy

2. **Production Build** (npm run build):
   - Environment variables are baked into the build
   - Frontend makes direct requests to the configured URLs
   - Backend must have CORS properly configured

### Troubleshooting

#### "Connection Refused" Errors

```
[vite] http proxy error: /api/v1/health
AggregateError [ECONNREFUSED]
```

**Solution:** Make sure the backend is running:
```bash
cd backend/build
./Main

# Verify it's listening
lsof -iTCP -sTCP:LISTEN -nP | grep -E "8081|8082"
```

#### "502 Bad Gateway" in Production

**Cause:** Frontend is configured to connect to localhost, but backend is on a different server.

**Solution:** Update `.env` with the correct backend URL and rebuild:
```bash
VITE_API_URL=http://your-backend-server:8081
VITE_WS_URL=ws://your-backend-server:8082
npm run build
```

#### CORS Errors in Production

**Cause:** Backend's CORS configuration doesn't allow the frontend's origin.

**Solution:** The backend already has CORS enabled for all origins (`Access-Control-Allow-Origin: *`). If you still see errors, check that:
1. You're using the correct protocol (http vs https)
2. The backend is actually running and accessible
3. No firewall is blocking the ports

### Testing the Configuration

1. **Check backend is running:**
   ```bash
   curl http://localhost:8081/api/v1/health
   ```
   Should return: `{"status":"ok"}`

2. **Check WebSocket:**
   ```bash
   # Install websocat: brew install websocat
   websocat ws://localhost:8082
   ```
   Should connect without errors.

3. **Start frontend:**
   ```bash
   npm run dev
   ```
   Open http://localhost:5173 - should show "Connected" status.

### Environment File Security

- ✅ `.env` is in `.gitignore` - won't be committed
- ✅ `.env.example` provides template for others
- ⚠️ Never commit API keys or secrets to `.env` files
- ℹ️ For production, use proper secrets management

### See Also

- [Native Build Guide](../../docs/02-setup/native-build-guide.md) - Running backend natively
- [Vite Environment Variables](https://vitejs.dev/guide/env-and-mode.html) - Official Vite documentation
