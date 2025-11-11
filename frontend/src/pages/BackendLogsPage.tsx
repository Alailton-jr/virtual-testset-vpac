import { useState, useEffect, useRef } from 'react'
import { Card, CardContent, CardDescription, CardHeader, CardTitle } from '@/components/ui/card'
import { Button } from '@/components/ui/button'
import { Badge } from '@/components/ui/badge'
import { Alert, AlertDescription } from '@/components/ui/alert'
import { Terminal, Trash2, Download, Pause, Play, AlertCircle, Wifi } from 'lucide-react'
import { api } from '@/lib/api'

interface LogEntry {
  timestamp: string
  level: 'INFO' | 'WARN' | 'ERROR' | 'DEBUG' | 'HTTP' | 'WS'
  message: string
}

export default function BackendLogsPage() {
  const [logs, setLogs] = useState<LogEntry[]>([])
  const [isPaused, setIsPaused] = useState(false)
  const [autoScroll, setAutoScroll] = useState(true)
  const [error, setError] = useState<string>('')
  const [connectionStatus, setConnectionStatus] = useState<'connected' | 'disconnected' | 'connecting'>('disconnected')
  const logsEndRef = useRef<HTMLDivElement>(null)
  const logsContainerRef = useRef<HTMLDivElement>(null)
  const wsRef = useRef<WebSocket | null>(null)

  // Connect to backend logs via WebSocket
  useEffect(() => {
    if (isPaused) return

    let ws: WebSocket | null = null
    let reconnectTimer: ReturnType<typeof setTimeout> | null = null

    const connect = () => {
      try {
        setConnectionStatus('connecting')
        
        ws = api.getLogsWebSocket()
        wsRef.current = ws

        ws.onopen = () => {
          setConnectionStatus('connected')
          setError('')
        }

        ws.onmessage = (event) => {
          try {
            const logEntry: LogEntry = JSON.parse(event.data)
            setLogs(prev => [...prev.slice(-499), logEntry]) // Keep last 500 logs
          } catch (err) {
            console.error('Failed to parse log message:', err)
          }
        }

        ws.onerror = (err) => {
          console.error('WebSocket error:', err)
          setConnectionStatus('disconnected')
          setError('Failed to connect to backend logs. Make sure the backend is running.')
        }

        ws.onclose = () => {
          setConnectionStatus('disconnected')
          setError('Connection to backend closed. Attempting to reconnect...')
          // Attempt to reconnect after 5 seconds
          reconnectTimer = setTimeout(() => {
            if (!isPaused) {
              connect()
            }
          }, 5000)
        }
      } catch (err) {
        console.error('Failed to create WebSocket:', err)
        setConnectionStatus('disconnected')
        setError('Failed to connect to backend. Make sure the backend server is running on port 8080.')
      }
    }

    connect()

    return () => {
      if (ws) {
        ws.close()
      }
      if (reconnectTimer) {
        clearTimeout(reconnectTimer)
      }
    }
  }, [isPaused])

  // Auto-scroll to bottom when new logs arrive
  useEffect(() => {
    if (autoScroll && logsEndRef.current) {
      logsEndRef.current.scrollIntoView({ behavior: 'smooth' })
    }
  }, [logs, autoScroll])

  // Detect manual scroll to disable auto-scroll
  const handleScroll = () => {
    if (!logsContainerRef.current) return
    const { scrollTop, scrollHeight, clientHeight } = logsContainerRef.current
    const isAtBottom = scrollHeight - scrollTop - clientHeight < 50
    setAutoScroll(isAtBottom)
  }

  const getLevelColor = (level: LogEntry['level']) => {
    switch (level) {
      case 'ERROR':
        return 'text-red-500 dark:text-red-400'
      case 'WARN':
        return 'text-yellow-500 dark:text-yellow-400'
      case 'INFO':
        return 'text-green-500 dark:text-green-400'
      case 'DEBUG':
        return 'text-blue-500 dark:text-blue-400'
      case 'HTTP':
        return 'text-cyan-500 dark:text-cyan-400'
      case 'WS':
        return 'text-magenta-500 dark:text-magenta-400'
      default:
        return 'text-gray-500 dark:text-gray-400'
    }
  }

  const getLevelBadgeVariant = (level: LogEntry['level']) => {
    switch (level) {
      case 'ERROR':
        return 'destructive'
      case 'WARN':
        return 'secondary'
      default:
        return 'default'
    }
  }

  const handleClearLogs = () => {
    setLogs([])
  }

  const handleDownloadLogs = () => {
    const logText = logs.map(log => 
      `[${new Date(log.timestamp).toLocaleString()}] ${log.level}: ${log.message}`
    ).join('\n')
    
    const blob = new Blob([logText], { type: 'text/plain' })
    const url = URL.createObjectURL(blob)
    const a = document.createElement('a')
    a.href = url
    a.download = `backend-logs-${new Date().toISOString().split('T')[0]}.txt`
    document.body.appendChild(a)
    a.click()
    document.body.removeChild(a)
    URL.revokeObjectURL(url)
  }

  const errorCount = logs.filter(l => l.level === 'ERROR').length
  const warnCount = logs.filter(l => l.level === 'WARN').length

  return (
    <div className="space-y-6">
      <div>
        <h1 className="text-3xl font-bold tracking-tight flex items-center gap-2">
          <Terminal className="h-8 w-8" />
          Backend Logs
        </h1>
        <p className="text-muted-foreground">
          Real-time backend server logs and terminal output
        </p>
      </div>

      {error && (
        <Alert variant="destructive">
          <AlertCircle className="h-4 w-4" />
          <AlertDescription>{error}</AlertDescription>
        </Alert>
      )}

      <div className="grid grid-cols-4 gap-4">
        <Card>
          <CardHeader className="pb-3">
            <CardTitle className="text-sm font-medium">Total Logs</CardTitle>
          </CardHeader>
          <CardContent>
            <div className="text-2xl font-bold">{logs.length}</div>
          </CardContent>
        </Card>
        <Card>
          <CardHeader className="pb-3">
            <CardTitle className="text-sm font-medium">Errors</CardTitle>
          </CardHeader>
          <CardContent>
            <div className="text-2xl font-bold text-red-500">{errorCount}</div>
          </CardContent>
        </Card>
        <Card>
          <CardHeader className="pb-3">
            <CardTitle className="text-sm font-medium">Warnings</CardTitle>
          </CardHeader>
          <CardContent>
            <div className="text-2xl font-bold text-yellow-500">{warnCount}</div>
          </CardContent>
        </Card>
        <Card>
          <CardHeader className="pb-3">
            <CardTitle className="text-sm font-medium">Status</CardTitle>
          </CardHeader>
          <CardContent>
            <div className="flex items-center gap-2">
              <Badge variant={isPaused ? 'secondary' : 'default'}>
                {isPaused ? 'Paused' : 'Live'}
              </Badge>
              {connectionStatus === 'connected' && (
                <span title="Connected to backend"><Wifi className="h-4 w-4 text-green-500" /></span>
              )}
              {connectionStatus === 'connecting' && (
                <span title="Connecting to backend..."><Wifi className="h-4 w-4 text-yellow-500 animate-pulse" /></span>
              )}
              {connectionStatus === 'disconnected' && (
                <span title="Disconnected"><AlertCircle className="h-4 w-4 text-red-500" /></span>
              )}
            </div>
          </CardContent>
        </Card>
      </div>

      <Card>
        <CardHeader>
          <div className="flex items-center justify-between">
            <div>
              <CardTitle>Terminal Output</CardTitle>
              <CardDescription>
                Backend server logs in real-time
              </CardDescription>
            </div>
            <div className="flex gap-2">
              <Button
                variant="outline"
                size="sm"
                onClick={() => setIsPaused(!isPaused)}
              >
                {isPaused ? (
                  <>
                    <Play className="mr-2 h-4 w-4" />
                    Resume
                  </>
                ) : (
                  <>
                    <Pause className="mr-2 h-4 w-4" />
                    Pause
                  </>
                )}
              </Button>
              <Button
                variant="outline"
                size="sm"
                onClick={handleDownloadLogs}
                disabled={logs.length === 0}
              >
                <Download className="mr-2 h-4 w-4" />
                Download
              </Button>
              <Button
                variant="outline"
                size="sm"
                onClick={handleClearLogs}
                disabled={logs.length === 0}
              >
                <Trash2 className="mr-2 h-4 w-4" />
                Clear
              </Button>
            </div>
          </div>
        </CardHeader>
        <CardContent>
          <div
            ref={logsContainerRef}
            onScroll={handleScroll}
            className="bg-black dark:bg-gray-950 rounded-lg p-4 h-[600px] overflow-y-auto font-mono text-sm"
          >
            {logs.length === 0 ? (
              <div className="flex items-center justify-center h-full text-gray-500">
                <div className="text-center">
                  <Terminal className="mx-auto h-12 w-12 mb-2 opacity-20" />
                  <p>No logs yet</p>
                  <p className="text-xs mt-1">Logs will appear here when the backend is running</p>
                </div>
              </div>
            ) : (
              <>
                {logs.map((log, index) => (
                  <div key={index} className="text-left py-1 hover:bg-gray-900 dark:hover:bg-gray-900/50 px-2 -mx-2 rounded">
                    <span className="text-gray-500 dark:text-gray-600">
                      {new Date(log.timestamp).toLocaleTimeString()}
                    </span>
                    <span className="mx-2">|</span>
                    <Badge 
                      variant={getLevelBadgeVariant(log.level)}
                      className="mr-2 font-mono text-xs"
                    >
                      {log.level}
                    </Badge>
                    <span className={getLevelColor(log.level)}>
                      {log.message}
                    </span>
                  </div>
                ))}
                <div ref={logsEndRef} />
              </>
            )}
          </div>
          {!autoScroll && (
            <div className="mt-2 text-center">
              <Button
                variant="outline"
                size="sm"
                onClick={() => {
                  setAutoScroll(true)
                  logsEndRef.current?.scrollIntoView({ behavior: 'smooth' })
                }}
              >
                ↓ Scroll to Bottom
              </Button>
            </div>
          )}
        </CardContent>
      </Card>

      {error && (
        <Alert variant="destructive">
          <AlertCircle className="h-4 w-4" />
          <AlertDescription>
            {error}
          </AlertDescription>
        </Alert>
      )}
      
      {connectionStatus === 'disconnected' && !error && (
        <Alert>
          <AlertCircle className="h-4 w-4" />
          <AlertDescription>
            <strong>Connection Required:</strong> To view backend logs, make sure the backend server is running 
            and implements a WebSocket endpoint at <code className="text-xs bg-muted px-1 py-0.5 rounded">ws://localhost:8080/ws/logs</code>.
          </AlertDescription>
        </Alert>
      )}
    </div>
  )
}
