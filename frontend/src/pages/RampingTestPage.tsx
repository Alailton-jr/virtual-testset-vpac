import { useState, useEffect } from 'react'
import { Card, CardContent, CardDescription, CardHeader, CardTitle } from '@/components/ui/card'
import { Button } from '@/components/ui/button'
import { Input } from '@/components/ui/input'
import { Label } from '@/components/ui/label'
import { Select, SelectContent, SelectItem, SelectTrigger, SelectValue } from '@/components/ui/select'
import { Badge } from '@/components/ui/badge'
import { Alert, AlertDescription } from '@/components/ui/alert'
import { Play, Square, AlertCircle } from 'lucide-react'
import { useStreamStore } from '@/stores/useStreamStore'
import { api } from '@/lib/api'
import type { RampResult } from '@/lib/types'

export default function RampingTestPage() {
  const { streams, fetchStreams } = useStreamStore()
  const [selectedStreamId, setSelectedStreamId] = useState<string>('')
  const [variable, setVariable] = useState<string>('I-A.mag')
  const [startValue, setStartValue] = useState('0')
  const [endValue, setEndValue] = useState('150')
  const [stepValue, setStepValue] = useState('5')
  const [durationSec, setDurationSec] = useState('0.5')
  const [isRunning, setIsRunning] = useState(false)
  const [results, setResults] = useState<RampResult | null>(null)
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

  const handleStartTest = async () => {
    if (!selectedStreamId) {
      setError('Please select a stream')
      return
    }

    setIsRunning(true)
    setError('')
    setResults(null)

    try {
      const result = await api.runRampingTest({
        streamId: selectedStreamId,
        variable,
        startValue: parseFloat(startValue),
        endValue: parseFloat(endValue),
        stepSize: parseFloat(stepValue),
        stepDuration: parseFloat(durationSec) * 1000, // Convert to milliseconds
        stopOnTrip: true,
        findDropoff: true,
      })

      setResults(result)
    } catch (err) {
      setError(err instanceof Error ? err.message : 'Failed to run ramping test')
    } finally {
      setIsRunning(false)
    }
  }

  const handleStopTest = async () => {
    if (!selectedStreamId) return

    try {
      await api.stopRampingTest(selectedStreamId)
      setIsRunning(false)
    } catch (err) {
      setError(err instanceof Error ? err.message : 'Failed to stop test')
    }
  }

  return (
    <div className="space-y-6">
      <div>
        <h1 className="text-3xl font-bold tracking-tight">Ramping Test</h1>
        <p className="text-muted-foreground">
          Automated ramping test for pickup/dropout determination
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
          <CardHeader>
            <CardTitle>Ramp Configuration</CardTitle>
            <CardDescription>
              Configure variable, range, step size, and stop conditions
            </CardDescription>
          </CardHeader>
          <CardContent className="space-y-4">
            <div className="grid gap-4 sm:grid-cols-2">
              <div className="space-y-2">
                <Label htmlFor="stream">Target Stream</Label>
                <Select value={selectedStreamId} onValueChange={setSelectedStreamId}>
                  <SelectTrigger id="stream"><SelectValue placeholder="Select stream" /></SelectTrigger>
                  <SelectContent>{streams.map((s) => <SelectItem key={s.id} value={s.id}>{s.name}</SelectItem>)}</SelectContent>
                </Select>
              </div>
              <div className="space-y-2">
                <Label htmlFor="variable">Variable</Label>
                <Select value={variable} onValueChange={setVariable}>
                  <SelectTrigger id="variable"><SelectValue /></SelectTrigger>
                  <SelectContent>
                    <SelectItem value="I-A.mag">Current A Magnitude</SelectItem>
                    <SelectItem value="I-B.mag">Current B Magnitude</SelectItem>
                    <SelectItem value="I-C.mag">Current C Magnitude</SelectItem>
                    <SelectItem value="V-A.mag">Voltage A Magnitude</SelectItem>
                    <SelectItem value="V-B.mag">Voltage B Magnitude</SelectItem>
                    <SelectItem value="V-C.mag">Voltage C Magnitude</SelectItem>
                    <SelectItem value="frequency">Frequency</SelectItem>
                  </SelectContent>
                </Select>
              </div>
            </div>
            <div className="grid gap-4 sm:grid-cols-3">
              <div className="space-y-2">
                <Label htmlFor="start">Start Value</Label>
                <Input id="start" type="number" value={startValue} onChange={(e) => setStartValue(e.target.value)} />
              </div>
              <div className="space-y-2">
                <Label htmlFor="end">End Value</Label>
                <Input id="end" type="number" value={endValue} onChange={(e) => setEndValue(e.target.value)} />
              </div>
              <div className="space-y-2">
                <Label htmlFor="step">Step Size</Label>
                <Input id="step" type="number" value={stepValue} onChange={(e) => setStepValue(e.target.value)} />
              </div>
            </div>
            <div className="grid gap-4 sm:grid-cols-2">
              <div className="space-y-2">
                <Label htmlFor="duration">Duration per Step (sec)</Label>
                <Input id="duration" type="number" step="0.1" value={durationSec} onChange={(e) => setDurationSec(e.target.value)} />
              </div>
            </div>
            <div className="flex gap-2">
              <Button 
                onClick={isRunning ? handleStopTest : handleStartTest} 
                disabled={!selectedStreamId} 
                className="flex-1"
                variant={isRunning ? 'destructive' : 'default'}
              >
                {isRunning ? (
                  <><Square className="mr-2 h-4 w-4" />Stop Test</>
                ) : (
                  <><Play className="mr-2 h-4 w-4" />Start Ramp</>
                )}
              </Button>
            </div>
          </CardContent>
        </Card>

        <Card>
          <CardHeader><CardTitle>Test KPIs</CardTitle><CardDescription>Key performance indicators</CardDescription></CardHeader>
          <CardContent className="space-y-3">
            <div className="flex items-center justify-between p-2 border rounded">
              <span className="text-sm font-medium">Pickup Value</span>
              <Badge variant={results?.pickupValue ? 'default' : 'outline'}>
                {results?.pickupValue ? `${results.pickupValue.toFixed(2)}` : 'N/A'}
              </Badge>
            </div>
            {results?.pickupTime && (
              <div className="flex items-center justify-between p-2 border rounded">
                <span className="text-sm font-medium">Pickup Time</span>
                <Badge variant="default">{results.pickupTime.toFixed(2)}ms</Badge>
              </div>
            )}
            <div className="flex items-center justify-between p-2 border rounded">
              <span className="text-sm font-medium">Dropoff Value</span>
              <Badge variant={results?.dropoffValue ? 'default' : 'outline'}>
                {results?.dropoffValue ? `${results.dropoffValue.toFixed(2)}` : 'N/A'}
              </Badge>
            </div>
            {results?.dropoffTime && (
              <div className="flex items-center justify-between p-2 border rounded">
                <span className="text-sm font-medium">Dropoff Time</span>
                <Badge variant="default">{results.dropoffTime.toFixed(2)}ms</Badge>
              </div>
            )}
            {results?.resetRatio && (
              <div className="flex items-center justify-between p-2 border rounded">
                <span className="text-sm font-medium">Reset Ratio</span>
                <Badge variant="default">{(results.resetRatio * 100).toFixed(1)}%</Badge>
              </div>
            )}
            <div className="flex items-center justify-between p-2 border rounded">
              <span className="text-sm font-medium">Trip Status</span>
              <Badge variant={results?.tripped ? 'destructive' : 'outline'}>
                {results?.tripped ? 'Tripped' : 'No Trip'}
              </Badge>
            </div>
          </CardContent>
        </Card>
      </div>
    </div>
  )
}
