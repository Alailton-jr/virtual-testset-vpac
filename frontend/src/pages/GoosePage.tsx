import { useState, useEffect } from 'react'
import { Card, CardContent, CardDescription, CardHeader, CardTitle } from '@/components/ui/card'
import { Button } from '@/components/ui/button'
import { Input } from '@/components/ui/input'
import { Label } from '@/components/ui/label'
import { Badge } from '@/components/ui/badge'
import { Alert, AlertDescription } from '@/components/ui/alert'
import { Search, Wifi, AlertCircle, Trash2 } from 'lucide-react'
import { api } from '@/lib/api'
import type { GooseMessage, GooseSubscription } from '@/lib/types'

export default function GoosePage() {
  const [isScanning, setIsScanning] = useState(false)
  const [messages, setMessages] = useState<GooseMessage[]>([])
  const [subscriptions, setSubscriptions] = useState<GooseSubscription[]>([])
  const [tripRule, setTripRule] = useState('RelayA_Trip/LLN0.Ind1.stVal == true')
  const [selectedAppId, setSelectedAppId] = useState('')
  const [selectedGoCBRef, setSelectedGoCBRef] = useState('')
  const [error, setError] = useState<string>('')
  const [loading, setLoading] = useState(true)

  // Fetch subscriptions on mount
  useEffect(() => {
    fetchSubscriptions()
  }, [])

  // Clear error after 5 seconds
  useEffect(() => {
    if (error) {
      const timer = setTimeout(() => setError(''), 5000)
      return () => clearTimeout(timer)
    }
  }, [error])

  const fetchSubscriptions = async () => {
    try {
      setLoading(true)
      const subs = await api.getGooseSubscriptions()
      setSubscriptions(subs)
      setError('')
    } catch (err) {
      setError(err instanceof Error ? err.message : 'Failed to fetch subscriptions')
    } finally {
      setLoading(false)
    }
  }

  const handleScan = async () => {
    setIsScanning(true)
    setError('')
    try {
      const response = await api.discoverGoose()
      setMessages(response.messages)
    } catch (err) {
      setError(err instanceof Error ? err.message : 'Failed to discover GOOSE messages')
      setMessages([])
    } finally {
      setIsScanning(false)
    }
  }

  const handleSelectMessage = (msg: GooseMessage) => {
    setSelectedAppId(msg.appId)
    setSelectedGoCBRef(msg.goCBRef)
  }

  const handleApplyTripRule = async () => {
    if (!selectedAppId || !selectedGoCBRef) {
      setError('Please select a GOOSE message first')
      return
    }

    if (!tripRule.trim()) {
      setError('Please enter a trip rule')
      return
    }

    try {
      await api.createGooseSubscription({
        appId: selectedAppId,
        goCBRef: selectedGoCBRef,
        tripRule,
        active: true,
      })
      setError('')
      setTripRule('RelayA_Trip/LLN0.Ind1.stVal == true')
      setSelectedAppId('')
      setSelectedGoCBRef('')
      await fetchSubscriptions()
    } catch (err) {
      setError(err instanceof Error ? err.message : 'Failed to create subscription')
    }
  }

  const handleDeleteSubscription = async (id: string) => {
    try {
      await api.deleteGooseSubscription(id)
      await fetchSubscriptions()
    } catch (err) {
      setError(err instanceof Error ? err.message : 'Failed to delete subscription')
    }
  }

  const handleToggleSubscription = async (sub: GooseSubscription) => {
    try {
      await api.updateGooseSubscription(sub.id, { active: !sub.active })
      await fetchSubscriptions()
    } catch (err) {
      setError(err instanceof Error ? err.message : 'Failed to toggle subscription')
    }
  }

  return (
    <div className="space-y-6">
      <div>
        <h1 className="text-3xl font-bold tracking-tight">GOOSE Monitor</h1>
        <p className="text-muted-foreground">Monitor and subscribe to IEC 61850 GOOSE messages</p>
      </div>

      {error && (
        <Alert variant="destructive">
          <AlertCircle className="h-4 w-4" />
          <AlertDescription>{error}</AlertDescription>
        </Alert>
      )}

      <div className="grid gap-6 lg:grid-cols-2">
        <Card>
          <CardHeader>
            <div className="flex items-center justify-between">
              <div><CardTitle>Message Discovery</CardTitle><CardDescription>Scan network for GOOSE messages</CardDescription></div>
              <Button onClick={handleScan} disabled={isScanning} size="sm">
                {isScanning ? <Wifi className="mr-2 h-4 w-4 animate-pulse" /> : <Search className="mr-2 h-4 w-4" />}
                {isScanning ? 'Scanning...' : 'Scan'}
              </Button>
            </div>
          </CardHeader>
          <CardContent>
            {messages.length === 0 ? (
              <p className="text-sm text-muted-foreground text-center py-4">No messages found. Click Scan to discover GOOSE traffic.</p>
            ) : (
              <div className="space-y-2">
                {messages.map((msg, idx) => (
                  <div 
                    key={idx} 
                    className={`p-3 border rounded-lg cursor-pointer hover:bg-muted/50 transition-colors ${
                      selectedAppId === msg.appId && selectedGoCBRef === msg.goCBRef ? 'bg-muted border-primary' : ''
                    }`}
                    onClick={() => handleSelectMessage(msg)}
                  >
                    <div className="flex items-center justify-between mb-2">
                      <span className="font-mono text-sm">{msg.goCBRef}</span>
                      <Badge variant="outline">{msg.lastSeen}</Badge>
                    </div>
                    <div className="text-xs text-muted-foreground space-y-1">
                      <p>App ID: {msg.appId}</p>
                      <p>MAC: {msg.macSrc}</p>
                      {msg.dataSet && <p>DataSet: {msg.dataSet}</p>}
                    </div>
                  </div>
                ))}
              </div>
            )}
          </CardContent>
        </Card>

        <Card>
          <CardHeader><CardTitle>Trip Rule Configuration</CardTitle><CardDescription>Define trip condition expression</CardDescription></CardHeader>
          <CardContent className="space-y-4">
            {selectedAppId && selectedGoCBRef && (
              <div className="p-3 bg-primary/10 border border-primary/20 rounded-lg text-sm">
                <p className="font-medium mb-1">Selected Message:</p>
                <p className="font-mono text-xs">{selectedGoCBRef}</p>
                <p className="text-xs text-muted-foreground">App ID: {selectedAppId}</p>
              </div>
            )}
            <div className="space-y-2">
              <Label htmlFor="trip-rule">Trip Rule Expression</Label>
              <Input 
                id="trip-rule" 
                value={tripRule} 
                onChange={(e) => setTripRule(e.target.value)} 
                placeholder="e.g., RelayA_Trip/LLN0.Ind1.stVal == true" 
                className="font-mono text-sm" 
              />
            </div>
            <div className="p-3 bg-muted rounded-lg text-sm space-y-1">
              <p className="font-medium">Examples:</p>
              <p className="text-xs text-muted-foreground">• RelayA_Trip/LLN0.Ind1.stVal == true</p>
              <p className="text-xs text-muted-foreground">• Breaker/XCBR1.Pos.stVal == 0</p>
            </div>
            <Button onClick={handleApplyTripRule} className="w-full" disabled={!selectedAppId || !selectedGoCBRef}>
              Apply Trip Rule
            </Button>
          </CardContent>
        </Card>
      </div>

      <Card>
        <CardHeader>
          <CardTitle>Active Subscriptions</CardTitle>
          <CardDescription>GOOSE subscriptions monitoring for trip conditions</CardDescription>
        </CardHeader>
        <CardContent>
          {loading ? (
            <p className="text-sm text-muted-foreground text-center py-4">Loading subscriptions...</p>
          ) : subscriptions.length === 0 ? (
            <p className="text-sm text-muted-foreground text-center py-4">No active subscriptions. Create one above.</p>
          ) : (
            <div className="space-y-2">
              {subscriptions.map((sub) => (
                <div key={sub.id} className="p-3 border rounded-lg">
                  <div className="flex items-center justify-between mb-2">
                    <span className="font-mono text-sm">{sub.goCBRef}</span>
                    <div className="flex items-center gap-2">
                      <Badge variant={sub.active ? 'default' : 'secondary'}>
                        {sub.active ? 'Active' : 'Inactive'}
                      </Badge>
                      <Button
                        variant="ghost"
                        size="sm"
                        onClick={() => handleToggleSubscription(sub)}
                      >
                        {sub.active ? 'Disable' : 'Enable'}
                      </Button>
                      <Button
                        variant="ghost"
                        size="sm"
                        onClick={() => handleDeleteSubscription(sub.id)}
                      >
                        <Trash2 className="h-4 w-4" />
                      </Button>
                    </div>
                  </div>
                  <div className="text-xs text-muted-foreground space-y-1">
                    <p>App ID: {sub.appId}</p>
                    <p className="font-mono">Rule: {sub.tripRule}</p>
                  </div>
                </div>
              ))}
            </div>
          )}
        </CardContent>
      </Card>
    </div>
  )
}
