import { useState, useEffect } from 'react'
import { Card, CardContent, CardDescription, CardHeader, CardTitle } from '@/components/ui/card'
import { Button } from '@/components/ui/button'
import { Input } from '@/components/ui/input'
import { Label } from '@/components/ui/label'
import { Badge } from '@/components/ui/badge'
import { Switch } from '@/components/ui/switch'
import { Select, SelectContent, SelectItem, SelectTrigger, SelectValue } from '@/components/ui/select'
import { RefreshCw, Check, AlertCircle, Network, Server, Settings as SettingsIcon } from 'lucide-react'
import { api } from '@/lib/api'

interface NetworkInterface {
  name: string
  active: boolean
  macAddress?: string
  ipAddress?: string
}

interface BackendSettings {
  host: string
  httpPort: number
  wsPort: number
  networkInterface: string
  autoReconnect: boolean
}

export default function SettingsPage() {
  const [settings, setSettings] = useState<BackendSettings>({
    host: 'localhost',
    httpPort: 8081,
    wsPort: 8082,
    networkInterface: '',
    autoReconnect: true,
  })

  const [interfaces, setInterfaces] = useState<NetworkInterface[]>([])
  const [loadingInterfaces, setLoadingInterfaces] = useState(false)
  const [testingConnection, setTestingConnection] = useState(false)
  const [connectionStatus, setConnectionStatus] = useState<'idle' | 'success' | 'error'>('idle')
  const [saveStatus, setSaveStatus] = useState<'idle' | 'saving' | 'saved'>('idle')

  // Load settings from localStorage on mount
  useEffect(() => {
    const savedSettings = localStorage.getItem('vts-backend-settings')
    if (savedSettings) {
      try {
        const parsed = JSON.parse(savedSettings)
        setSettings(parsed)
      } catch (error) {
        console.error('Failed to load settings:', error)
      }
    }
  }, [])

  const scanInterfaces = async () => {
    setLoadingInterfaces(true)
    try {
      // Call backend endpoint to get available network interfaces
      const response = await api.getNetworkInterfaces()
      setInterfaces(response.interfaces)
    } catch (error) {
      console.error('Failed to scan interfaces:', error)
      // Fallback to mock data if backend call fails
      const mockInterfaces: NetworkInterface[] = [
        { name: 'en0', active: true, macAddress: '00:11:22:33:44:55', ipAddress: '192.168.0.148' },
        { name: 'en1', active: false, macAddress: '00:11:22:33:44:56', ipAddress: undefined },
      ]
      setInterfaces(mockInterfaces)
    } finally {
      setLoadingInterfaces(false)
    }
  }

  const testConnection = async () => {
    setTestingConnection(true)
    setConnectionStatus('idle')
    
    try {
      // Test if we can reach the backend with current settings
      const response = await fetch(`http://${settings.host}:${settings.httpPort}/api/v1/health`)
      if (response.ok) {
        setConnectionStatus('success')
      } else {
        setConnectionStatus('error')
      }
    } catch (error) {
      console.error('Connection test failed:', error)
      setConnectionStatus('error')
    } finally {
      setTestingConnection(false)
    }
  }

  const saveSettings = () => {
    setSaveStatus('saving')
    
    // Save to localStorage
    localStorage.setItem('vts-backend-settings', JSON.stringify(settings))
    
    // Update API client base URL if needed
    // This would require modifying the API client to support dynamic base URLs
    
    setTimeout(() => {
      setSaveStatus('saved')
      setTimeout(() => setSaveStatus('idle'), 2000)
    }, 500)
  }

  const resetToDefaults = () => {
    const defaults: BackendSettings = {
      host: 'localhost',
      httpPort: 8081,
      wsPort: 8082,
      networkInterface: '',
      autoReconnect: true,
    }
    setSettings(defaults)
    localStorage.removeItem('vts-backend-settings')
  }

  return (
    <div className="space-y-6">
      <div>
        <h1 className="text-3xl font-bold tracking-tight">Settings</h1>
        <p className="text-muted-foreground">
          Configure backend connection and server preferences
        </p>
      </div>

      <div className="grid gap-6">
        {/* Backend Connection Settings */}
        <Card>
          <CardHeader>
            <div className="flex items-center gap-2">
              <Server className="h-5 w-5" />
              <CardTitle>Backend Connection</CardTitle>
            </div>
            <CardDescription>
              Configure REST API and WebSocket connection settings
            </CardDescription>
          </CardHeader>
          <CardContent className="space-y-6">
            <div className="grid gap-4 md:grid-cols-2">
              <div className="space-y-2">
                <Label htmlFor="host">Backend Host/IP Address</Label>
                <Input
                  id="host"
                  placeholder="localhost or 192.168.0.100"
                  value={settings.host}
                  onChange={(e) => setSettings({ ...settings, host: e.target.value })}
                />
                <p className="text-xs text-muted-foreground">
                  Hostname or IP address of the backend server
                </p>
              </div>

              <div className="space-y-2">
                <Label htmlFor="httpPort">HTTP API Port</Label>
                <Input
                  id="httpPort"
                  type="number"
                  min="1"
                  max="65535"
                  value={settings.httpPort}
                  onChange={(e) => setSettings({ ...settings, httpPort: parseInt(e.target.value) || 8081 })}
                />
                <p className="text-xs text-muted-foreground">
                  Port for REST API (default: 8081)
                </p>
              </div>

              <div className="space-y-2">
                <Label htmlFor="wsPort">WebSocket Port</Label>
                <Input
                  id="wsPort"
                  type="number"
                  min="1"
                  max="65535"
                  value={settings.wsPort}
                  onChange={(e) => setSettings({ ...settings, wsPort: parseInt(e.target.value) || 8082 })}
                />
                <p className="text-xs text-muted-foreground">
                  Port for WebSocket connections (default: 8082)
                </p>
              </div>

              <div className="space-y-2">
                <Label>Connection Status</Label>
                <div className="flex items-center gap-2">
                  <Button
                    variant="outline"
                    size="sm"
                    onClick={testConnection}
                    disabled={testingConnection}
                  >
                    {testingConnection ? (
                      <RefreshCw className="h-4 w-4 mr-2 animate-spin" />
                    ) : (
                      <Network className="h-4 w-4 mr-2" />
                    )}
                    Test Connection
                  </Button>
                  {connectionStatus === 'success' && (
                    <Badge className="bg-green-600">
                      <Check className="h-3 w-3 mr-1" />
                      Connected
                    </Badge>
                  )}
                  {connectionStatus === 'error' && (
                    <Badge variant="destructive">
                      <AlertCircle className="h-3 w-3 mr-1" />
                      Failed
                    </Badge>
                  )}
                </div>
              </div>
            </div>

            <div className="flex items-center justify-between rounded-lg border p-4">
              <div className="space-y-0.5">
                <Label>Auto-Reconnect</Label>
                <p className="text-sm text-muted-foreground">
                  Automatically reconnect when connection is lost
                </p>
              </div>
              <Switch
                checked={settings.autoReconnect}
                onCheckedChange={(checked) => setSettings({ ...settings, autoReconnect: checked })}
              />
            </div>
          </CardContent>
        </Card>

        {/* Network Interface Selection */}
        <Card>
          <CardHeader>
            <div className="flex items-center gap-2">
              <Network className="h-5 w-5" />
              <CardTitle>Network Interface</CardTitle>
            </div>
            <CardDescription>
              Select which network interface the backend should use for IEC 61850 traffic
            </CardDescription>
          </CardHeader>
          <CardContent className="space-y-4">
            <div className="flex items-center gap-2">
              <Button
                variant="outline"
                onClick={scanInterfaces}
                disabled={loadingInterfaces}
              >
                {loadingInterfaces ? (
                  <RefreshCw className="h-4 w-4 mr-2 animate-spin" />
                ) : (
                  <RefreshCw className="h-4 w-4 mr-2" />
                )}
                Scan Interfaces
              </Button>
              <p className="text-sm text-muted-foreground">
                {interfaces.length > 0 ? `Found ${interfaces.length} interface(s)` : 'Click to scan available interfaces'}
              </p>
            </div>

            {interfaces.length > 0 && (
              <div className="space-y-2">
                <Label htmlFor="interface">Select Network Interface</Label>
                <Select
                  value={settings.networkInterface}
                  onValueChange={(value) => setSettings({ ...settings, networkInterface: value })}
                >
                  <SelectTrigger id="interface">
                    <SelectValue placeholder="Select an interface" />
                  </SelectTrigger>
                  <SelectContent>
                    {interfaces.map((iface) => (
                      <SelectItem key={iface.name} value={iface.name}>
                        <div className="flex items-center gap-2">
                          <span className="font-mono">{iface.name}</span>
                          {iface.active && (
                            <Badge variant="outline" className="text-green-600 border-green-600">
                              Active
                            </Badge>
                          )}
                          {iface.ipAddress && (
                            <span className="text-xs text-muted-foreground">
                              {iface.ipAddress}
                            </span>
                          )}
                        </div>
                      </SelectItem>
                    ))}
                  </SelectContent>
                </Select>

                {settings.networkInterface && (
                  <div className="rounded-lg border p-3 space-y-1">
                    {interfaces.find(i => i.name === settings.networkInterface) && (
                      <>
                        <div className="flex justify-between text-sm">
                          <span className="text-muted-foreground">Interface:</span>
                          <span className="font-mono">{settings.networkInterface}</span>
                        </div>
                        {interfaces.find(i => i.name === settings.networkInterface)?.macAddress && (
                          <div className="flex justify-between text-sm">
                            <span className="text-muted-foreground">MAC Address:</span>
                            <span className="font-mono text-xs">
                              {interfaces.find(i => i.name === settings.networkInterface)?.macAddress}
                            </span>
                          </div>
                        )}
                        {interfaces.find(i => i.name === settings.networkInterface)?.ipAddress && (
                          <div className="flex justify-between text-sm">
                            <span className="text-muted-foreground">IP Address:</span>
                            <span className="font-mono">
                              {interfaces.find(i => i.name === settings.networkInterface)?.ipAddress}
                            </span>
                          </div>
                        )}
                      </>
                    )}
                  </div>
                )}
              </div>
            )}

            <div className="rounded-lg bg-muted p-4 space-y-2">
              <div className="flex items-start gap-2">
                <AlertCircle className="h-4 w-4 mt-0.5 text-muted-foreground" />
                <div className="space-y-1">
                  <p className="text-sm font-medium">Important Notes:</p>
                  <ul className="text-sm text-muted-foreground space-y-1 list-disc list-inside">
                    <li>The backend must be running with root privileges (sudo) to access raw network interfaces</li>
                    <li>Only active interfaces with IFF_UP and IFF_RUNNING flags can send/receive packets</li>
                    <li>On macOS, ensure the backend is not using --no-net mode</li>
                  </ul>
                </div>
              </div>
            </div>
          </CardContent>
        </Card>

        {/* Server Configuration */}
        <Card>
          <CardHeader>
            <div className="flex items-center gap-2">
              <SettingsIcon className="h-5 w-5" />
              <CardTitle>Server Configuration</CardTitle>
            </div>
            <CardDescription>
              Advanced server settings and preferences
            </CardDescription>
          </CardHeader>
          <CardContent>
            <div className="rounded-lg border p-4">
              <p className="text-sm text-muted-foreground">
                Additional server configuration options will be available in future updates.
                This may include:
              </p>
              <ul className="mt-2 text-sm text-muted-foreground space-y-1 list-disc list-inside">
                <li>Sample rate configuration</li>
                <li>Buffer size settings</li>
                <li>Logging level and output</li>
                <li>Performance optimization options</li>
              </ul>
            </div>
          </CardContent>
        </Card>

        {/* Action Buttons */}
        <div className="flex items-center justify-between">
          <Button variant="outline" onClick={resetToDefaults}>
            Reset to Defaults
          </Button>
          <div className="flex items-center gap-2">
            {saveStatus === 'saved' && (
              <span className="text-sm text-green-600 flex items-center gap-1">
                <Check className="h-4 w-4" />
                Settings saved
              </span>
            )}
            <Button onClick={saveSettings} disabled={saveStatus === 'saving'}>
              {saveStatus === 'saving' ? 'Saving...' : 'Save Settings'}
            </Button>
          </div>
        </div>
      </div>
    </div>
  )
}
