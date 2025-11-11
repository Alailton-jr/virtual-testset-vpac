import { useState, useEffect } from 'react'
import { Card, CardContent, CardDescription, CardHeader, CardTitle } from '@/components/ui/card'
import { Button } from '@/components/ui/button'
import { Input } from '@/components/ui/input'
import { Label } from '@/components/ui/label'
import { Select, SelectContent, SelectItem, SelectTrigger, SelectValue } from '@/components/ui/select'
import { Badge } from '@/components/ui/badge'
import { Alert, AlertDescription } from '@/components/ui/alert'
import { Play, Loader2, AlertCircle } from 'lucide-react'
import { useStreamStore } from '@/stores/useStreamStore'
import { api } from '@/lib/api'
import type { OvercurrentTestResult } from '@/lib/types'

export default function OvercurrentTestPage() {
  const { streams, fetchStreams } = useStreamStore()
  const [selectedStreamId, setSelectedStreamId] = useState<string>('')
  const [pickup, setPickup] = useState('5.0')
  const [timeDial, setTimeDial] = useState('5')
  const [curveType, setCurveType] = useState<'IEC_SI' | 'IEC_VI' | 'IEC_EI' | 'IEEE_MI' | 'IEEE_VI' | 'IEEE_EI'>('IEC_SI')
  const [results, setResults] = useState<OvercurrentTestResult[]>([])
  const [isRunning, setIsRunning] = useState(false)
  const [error, setError] = useState<string>('')

  // Fetch streams on mount
  useEffect(() => {
    fetchStreams()
  }, [fetchStreams])

  // Clear error after 5 seconds
  useEffect(() => {
    if (error) {
      const timer = setTimeout(() => setError(''), 5000)
      return () => clearTimeout(timer)
    }
  }, [error])

  const handleRunTest = async () => {
    if (!selectedStreamId) {
      setError('Please select a stream')
      return
    }

    setIsRunning(true)
    setError('')
    setResults([])

    try {
      const testResults = await api.runOvercurrentTest({
        streamId: selectedStreamId,
        pickup: parseFloat(pickup),
        tms: parseFloat(timeDial),
        curve: curveType,
        testPoints: [1.2, 2, 4, 10],  // Multiples of pickup
      })

      setResults(testResults)
    } catch (err) {
      setError(err instanceof Error ? err.message : 'Failed to run overcurrent test')
    } finally {
      setIsRunning(false)
    }
  }

  return (
    <div className="space-y-6">
      <div>
        <h1 className="text-3xl font-bold tracking-tight">Overcurrent 50/51 Test</h1>
        <p className="text-muted-foreground">
          Automated testing for overcurrent protection relays
        </p>
      </div>

      {error && (
        <Alert variant="destructive">
          <AlertCircle className="h-4 w-4" />
          <AlertDescription>{error}</AlertDescription>
        </Alert>
      )}

      <div className="grid gap-6 lg:grid-cols-3">
        <Card className="lg:col-span-2">
          <CardHeader><CardTitle>Test Configuration</CardTitle><CardDescription>Configure pickup, time dial, curve type, and test points</CardDescription></CardHeader>
          <CardContent className="space-y-4">
            <div className="space-y-2">
              <Label htmlFor="stream">Target Stream</Label>
              <Select value={selectedStreamId} onValueChange={setSelectedStreamId}>
                <SelectTrigger id="stream"><SelectValue placeholder="Select stream" /></SelectTrigger>
                <SelectContent>{streams.map((s) => <SelectItem key={s.id} value={s.id}>{s.name}</SelectItem>)}</SelectContent>
              </Select>
            </div>
            <div className="grid gap-4 sm:grid-cols-3">
              <div className="space-y-2">
                <Label htmlFor="pickup">Pickup (A)</Label>
                <Input id="pickup" type="number" step="0.1" value={pickup} onChange={(e) => setPickup(e.target.value)} />
              </div>
              <div className="space-y-2">
                <Label htmlFor="timeDial">Time Dial/TMS</Label>
                <Input id="timeDial" type="number" step="0.1" value={timeDial} onChange={(e) => setTimeDial(e.target.value)} />
              </div>
              <div className="space-y-2">
                <Label htmlFor="curve">Curve Type</Label>
                <Select value={curveType} onValueChange={(val) => setCurveType(val as typeof curveType)}>
                  <SelectTrigger id="curve"><SelectValue /></SelectTrigger>
                  <SelectContent>
                    <SelectItem value="IEC_SI">IEC Standard Inverse</SelectItem>
                    <SelectItem value="IEC_VI">IEC Very Inverse</SelectItem>
                    <SelectItem value="IEC_EI">IEC Extremely Inverse</SelectItem>
                    <SelectItem value="IEEE_MI">IEEE Moderately Inverse</SelectItem>
                    <SelectItem value="IEEE_VI">IEEE Very Inverse</SelectItem>
                    <SelectItem value="IEEE_EI">IEEE Extremely Inverse</SelectItem>
                  </SelectContent>
                </Select>
              </div>
            </div>
            <Button onClick={handleRunTest} disabled={isRunning || !selectedStreamId} className="w-full">
              {isRunning ? <><Loader2 className="mr-2 h-4 w-4 animate-spin" />Running Test...</> : <><Play className="mr-2 h-4 w-4" />Run Test</>}
            </Button>
            {results.length > 0 && (
              <div className="space-y-2">
                <Label>Test Results</Label>
                {results.map((r, idx) => (
                  <div key={idx} className="flex items-center justify-between p-2 border rounded text-sm">
                    <span className="font-mono">{r.current.toFixed(1)}A ({r.multiple.toFixed(1)}×)</span>
                    <span className="text-muted-foreground text-xs">
                      Exp: {r.expectedTime.toFixed(2)}s
                      {r.actualTime !== undefined && ` / Act: ${r.actualTime.toFixed(2)}s`}
                    </span>
                    <Badge variant={r.pass ? 'default' : 'destructive'}>{r.pass ? 'PASS' : 'FAIL'}</Badge>
                  </div>
                ))}
              </div>
            )}
          </CardContent>
        </Card>

        <Card>
          <CardHeader><CardTitle>Time-Current Curve</CardTitle><CardDescription>Expected vs actual</CardDescription></CardHeader>
          <CardContent>
            <div className="h-80 flex items-center justify-center border rounded-lg bg-muted/20">
              <p className="text-sm text-muted-foreground">TCC curve visualization coming soon...</p>
            </div>
          </CardContent>
        </Card>
      </div>
    </div>
  )
}
