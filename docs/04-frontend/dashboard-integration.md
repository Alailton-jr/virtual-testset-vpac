# Dashboard Real-Time Backend Integration

## Changes Summary

The Dashboard component has been updated to fetch real-time data from the backend API instead of displaying mocked static data.

## Updated Files

### 1. `frontend/src/lib/api.ts`

Added new API methods for system status:

```typescript
// Health Check
async healthCheck(): Promise<{ status: string; timestamp: number; version: string }>

// System Status
async getSequenceStatus(): Promise<{ running: boolean; currentStep?: number; totalSteps?: number }>
async getAnalyzerStatus(): Promise<{ active: boolean; streamId?: string }>
```

### 2. `frontend/src/pages/Dashboard.tsx`

**Major Updates:**

1. **Added State Management:**
   - `DashboardStats` interface for typed state
   - Loading and error states
   - Real-time data fetching with `useEffect`

2. **Implemented Real API Calls:**
   - `api.healthCheck()` - Backend status and version
   - `api.getStreams()` - Active SV publishers
   - `api.getGooseSubscriptions()` - GOOSE subscriptions count
   - `api.getSequenceStatus()` - Running sequences
   - `api.getAnalyzerStatus()` - Running tests

3. **Auto-Refresh:**
   - Data refreshes every 5 seconds
   - Graceful error handling
   - Loading indicators

## Dashboard Metrics

### Real-Time Counters

| Metric | Source | Description |
|--------|--------|-------------|
| **Active Streams** | `/api/v1/streams` | Count of running SV publishers |
| **GOOSE Subscriptions** | `/api/v1/goose/subscriptions` | Active GOOSE message subscriptions |
| **Running Tests** | `/api/v1/analyzer/status` | Tests currently in progress |
| **Sequences** | `/api/v1/sequences/status` | Running test sequences |

### System Status

| Field | Source | Description |
|-------|--------|-------------|
| **Backend** | `/api/v1/health` | Connection status (Connected/Disconnected) |
| **Backend Version** | `/api/v1/health` | Backend API version |
| **Active Streams** | `/api/v1/streams` | Number of active publishers |
| **Sample Rate** | Static | 4000 Hz (from configuration) |

## Features

### ✅ Real-Time Updates
- Dashboard fetches data every 5 seconds
- Automatic retry on connection failure
- No page refresh needed

### ✅ Error Handling
- Graceful degradation when backend is unavailable
- Error messages displayed to user
- Individual metric failures don't break the dashboard

### ✅ Loading States
- "..." indicator while fetching data
- Prevents showing stale data
- Smooth user experience

### ✅ Backend Health Monitoring
- Green "Connected" when backend is healthy
- Red "Disconnected" when backend is unreachable
- Version information display

## API Error Handling

The implementation uses `Promise.allSettled()` to handle multiple API calls:

```typescript
const [health, streams, gooseData, sequenceStatus, analyzerStatus] = await Promise.allSettled([
  api.healthCheck(),
  api.getStreams(),
  api.getGooseSubscriptions(),
  api.getSequenceStatus(),
  api.getAnalyzerStatus(),
])
```

**Benefits:**
- If one endpoint fails, others still work
- Individual metrics show "0" on failure
- Dashboard remains functional even with partial backend availability

## Testing

### Test the Dashboard

1. **Start both containers:**
   ```bash
   docker compose -f docker/docker-compose.macos.yml up -d
   ```

2. **Access the frontend:**
   ```
   http://localhost:5173
   ```

3. **Verify metrics:**
   - Backend status should show "Connected" (green)
   - Version should display (e.g., "1.0.0")
   - Metrics should show real counts from backend

### Test Error Handling

1. **Stop the backend:**
   ```bash
   docker stop vts-backend-macos
   ```

2. **Check dashboard:**
   - Backend status should show "Disconnected" (red)
   - Error message should appear at top
   - Counters show last known values or "0"

3. **Restart backend:**
   ```bash
   docker start vts-backend-macos
   ```

4. **Dashboard auto-recovers:**
   - Within 5 seconds, backend status returns to "Connected"
   - Metrics update with fresh data

## Future Enhancements

### Potential Improvements

1. **WebSocket Integration:**
   - Real-time push updates instead of polling
   - Immediate notification of state changes
   - Lower latency for metric updates

2. **Historical Data:**
   - Chart showing stream count over time
   - Test execution history
   - Performance metrics graphs

3. **Configurable Refresh Rate:**
   - User setting for update frequency
   - Pause/resume auto-refresh
   - Manual refresh button

4. **Detailed Metrics:**
   - Per-stream information in tooltips
   - Click-through to detailed views
   - Test result summaries

5. **Alerts/Notifications:**
   - Toast notifications for state changes
   - Warning badges for issues
   - Browser notifications for critical events

## Architecture

### Data Flow

```
Dashboard Component
    ↓
    ├─→ useEffect Hook (on mount + every 5s)
    │       ↓
    │   API Client (api.ts)
    │       ↓
    │   Backend REST API (http://localhost:8080/api/v1/*)
    │       ↓
    │   Response Processing
    │       ↓
    └─→ State Update (setStats)
            ↓
        UI Re-render
```

### Component Structure

```typescript
Dashboard
  ├─ Stats State (activeStreams, gooseSubscriptions, etc.)
  ├─ Loading State
  ├─ Error State
  └─ useEffect (fetch + interval)
      └─ Promise.allSettled (parallel API calls)
          ├─ healthCheck()
          ├─ getStreams()
          ├─ getGooseSubscriptions()
          ├─ getSequenceStatus()
          └─ getAnalyzerStatus()
```

## Backend Endpoints Used

### Core Endpoints

1. **Health Check**
   - `GET /api/v1/health`
   - Returns: `{ status: "ok", timestamp: number, version: string }`

2. **Streams List**
   - `GET /api/v1/streams`
   - Returns: `{ streams: Stream[] }`

3. **GOOSE Subscriptions**
   - `GET /api/v1/goose/subscriptions`
   - Returns: `{ subscriptions: GooseSubscription[] }`

4. **Sequence Status**
   - `GET /api/v1/sequences/status`
   - Returns: `{ running: boolean, currentStep?: number, totalSteps?: number }`

5. **Analyzer Status**
   - `GET /api/v1/analyzer/status`
   - Returns: `{ active: boolean, streamId?: string }`

## Browser Compatibility

- ✅ Chrome/Edge (Chromium)
- ✅ Firefox
- ✅ Safari
- ✅ Mobile browsers

**Requirements:**
- ES2020+ (async/await, Promise.allSettled)
- Fetch API
- Modern React hooks

## Performance Considerations

### Optimization Strategies

1. **Parallel API Calls:**
   - All endpoints called simultaneously
   - Total wait time = slowest endpoint (not sum of all)

2. **Interval Cleanup:**
   - `clearInterval` on component unmount
   - Prevents memory leaks

3. **Error Isolation:**
   - Individual endpoint failures don't cascade
   - Dashboard remains functional

4. **Minimal Re-renders:**
   - State updates batched
   - Only changed values trigger re-render

### Network Traffic

- **Initial Load:** ~5 API calls
- **Per Refresh (5s):** ~5 API calls
- **Total:** ~60 API calls/minute
- **Data Volume:** ~1-2 KB per refresh (minimal JSON)

## Troubleshooting

### Dashboard Shows "Disconnected"

**Causes:**
1. Backend container not running
2. Network configuration issue
3. CORS policy blocking requests

**Solutions:**
```bash
# Check backend status
docker ps | grep vts-backend

# View backend logs
docker logs vts-backend-macos

# Restart backend
docker restart vts-backend-macos
```

### Metrics Show "0" but Backend is Connected

**Causes:**
1. No streams created yet
2. API endpoints returning empty data
3. Backend initialization incomplete

**Solutions:**
- Create test streams via SV Publishers page
- Check backend logs for errors
- Verify backend API responses with curl

### Error Message Displayed

**Check Browser Console:**
```javascript
// Should see API calls
fetch http://localhost:8080/api/v1/health
fetch http://localhost:8080/api/v1/streams
```

**Common Issues:**
- CORS policy errors → Check backend CORS configuration
- Network errors → Verify backend is accessible
- 404 errors → API endpoint not implemented

---

**Implementation Date:** November 11, 2025  
**Status:** ✅ Complete and Tested  
**Backend Version:** 1.0.0  
**Frontend Version:** 0.0.0
