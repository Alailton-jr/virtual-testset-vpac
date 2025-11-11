# Backend Monitoring Tools - Setup Complete ✅

## What Was Created

I've created a comprehensive monitoring system for your Virtual TestSet backend that allows you to easily inspect logs, errors, and debug issues.

### 📁 New Files Created

1. **`backend/scripts/monitor.sh`** - Main monitoring script
   - Starts backend with color-coded real-time logs
   - Auto-builds if executable is missing
   - Saves all logs to `.logs/` directory
   - Shows ERROR (red), WARN (yellow), INFO (green), DEBUG (blue)

2. **`backend/scripts/watch-logs.sh`** - Log viewer
   - Watch logs in a separate terminal
   - Real-time tail with color coding
   - Useful for monitoring while backend runs elsewhere

3. **`backend/scripts/status.sh`** - Status checker
   - Quick health check for backend
   - Shows if server is running, port, PID
   - Tests HTTP endpoint
   - Displays last 10 log entries

4. **`backend/MONITORING.md`** - Complete documentation
   - Detailed usage guide
   - Debugging scenarios
   - Troubleshooting tips
   - VS Code integration instructions

5. **`backend/MONITORING-QUICKREF.md`** - Quick reference card
   - One-page cheat sheet
   - Common commands
   - Color guide
   - Quick troubleshooting

---

## 🚀 How to Use

### Quick Start (Most Common)

```bash
cd backend
./scripts/monitor.sh
```

This will:
- Build the backend if needed
- Start the server
- Show color-coded logs in real-time
- Save logs to `.logs/backend-TIMESTAMP.log`

**Press Ctrl+C to stop**

### Multi-Terminal Setup (Recommended for Development)

**Terminal 1: Backend**
```bash
cd backend
./scripts/monitor.sh
```

**Terminal 2: Frontend**
```bash
cd frontend
npm run dev
```

**Terminal 3: Watch Logs (optional)**
```bash
cd backend
./scripts/watch-logs.sh
```

### Check Backend Status

```bash
cd backend
./scripts/status.sh
```

Shows:
- ✅ If backend is running
- ✅ Port and PID
- ✅ HTTP health check
- ✅ Recent log entries

---

## 🎨 Color Coding

The scripts use colors to help you quickly identify issues:

| Color | Type | What It Means |
|-------|------|---------------|
| 🔴 **Red** | ERROR/FATAL | Something broke - needs immediate attention |
| 🟡 **Yellow** | WARN | Warning - might cause issues later |
| 🟢 **Green** | INFO/Success | Everything OK, operation successful |
| 🔵 **Blue** | DEBUG | Detailed debug information |
| 🟣 **Magenta** | WebSocket | WebSocket connections and messages |
| 🔷 **Cyan** | HTTP | HTTP requests (GET, POST, PUT, DELETE) |

---

## 📝 Log Files

All logs are automatically saved to:

```
backend/.logs/backend-20241111-143025.log
```

Format: `backend-YYYYMMDD-HHMMSS.log`

### View Historical Logs

```bash
# List all logs
ls -lh backend/.logs/

# View a specific log
cat backend/.logs/backend-20241111-143025.log

# Search for errors
grep -i error backend/.logs/*.log

# Watch latest log
tail -f backend/.logs/$(ls -t backend/.logs/ | head -n 1)
```

---

## 🐛 Debugging Examples

### Example 1: Backend Won't Start

```bash
cd backend
./scripts/monitor.sh
```

**Look for:**
- 🔴 Red ERROR messages
- Port already in use: `lsof -i :8080`
- Missing dependencies
- Build failures

### Example 2: Frontend Can't Connect

```bash
# Terminal 1: Start backend
cd backend
./scripts/monitor.sh

# Terminal 2: Check status
cd backend
./scripts/status.sh
```

**Should see:**
```
✓ Process running on port 8080
✓ HTTP API is responding
```

### Example 3: API Returns Errors

```bash
# Terminal 1: Backend with logs
cd backend
./scripts/monitor.sh

# Terminal 2: Make request
curl http://localhost:8080/api/streams
```

**Watch Terminal 1 for:**
- 🔷 Cyan HTTP request log
- 🔴 Red error messages if something fails

### Example 4: WebSocket Issues

```bash
# Terminal 1: Backend
cd backend
./scripts/monitor.sh

# Opens browser to frontend
# Try connecting WebSocket feature
```

**Look for in Terminal 1:**
- 🟣 Magenta WebSocket connection logs
- 🟢 Green "Connection established" messages
- 🔴 Red connection errors

---

## 🛑 Stopping the Backend

### Method 1: Graceful Stop
If running in terminal:
```bash
Press Ctrl+C
```

### Method 2: Using Status Script
```bash
cd backend
./scripts/status.sh  # Note the PID
kill <PID>
```

### Method 3: Kill by Port
```bash
lsof -i :8080  # Find PID
kill -9 <PID>
```

---

## 📚 Documentation

- **Quick Reference:** `backend/MONITORING-QUICKREF.md` - One-page cheat sheet
- **Full Guide:** `backend/MONITORING.md` - Complete documentation with examples

---

## 💡 Pro Tips

1. **Always start backend before frontend**
   ```bash
   # Terminal 1
   cd backend && ./scripts/monitor.sh
   
   # Terminal 2 (after backend starts)
   cd frontend && npm run dev
   ```

2. **Keep logs terminal visible** when testing new features

3. **Red = stop and investigate** - don't ignore error messages

4. **Use Ctrl+C** to gracefully stop the server (not kill -9)

5. **Check status before debugging:**
   ```bash
   cd backend && ./scripts/status.sh
   ```

6. **Archive old logs periodically:**
   ```bash
   cd backend/.logs
   tar -czf archive-$(date +%Y%m%d).tar.gz backend-*.log
   rm backend-*.log
   ```

---

## 🎯 Next Steps

1. **Test the monitoring system:**
   ```bash
   cd backend
   ./scripts/monitor.sh
   ```

2. **Start developing with visible logs:**
   - Terminal 1: `cd backend && ./scripts/monitor.sh`
   - Terminal 2: `cd frontend && npm run dev`

3. **Bookmark the quick reference:**
   - Keep `MONITORING-QUICKREF.md` open in a terminal or editor

4. **When something goes wrong:**
   - Check the color-coded logs
   - Look for 🔴 red ERROR messages
   - Review the full log file in `.logs/`

---

## ✅ Benefits

✅ **Real-time visibility** - See exactly what the backend is doing  
✅ **Color-coded logs** - Quickly identify errors and warnings  
✅ **Persistent logs** - All output saved to files for review  
✅ **Easy debugging** - Clear error messages and stack traces  
✅ **Status checking** - Quick health checks without reading logs  
✅ **Multiple workflows** - Works in one terminal or split across many  

---

**You now have a complete monitoring system for your backend! 🎉**

When something goes wrong, you'll be able to see exactly what happened in the logs with clear, color-coded output.
