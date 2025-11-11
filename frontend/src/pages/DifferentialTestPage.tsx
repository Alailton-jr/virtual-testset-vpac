import { useState, useEffect } from 'react'
import { Card, CardContent, CardDescription, CardHeader, CardTitle } from '@/components/ui/card'
import { Button } from '@/components/ui/button'
import { Input } from '@/components/ui/input'
import { Label } from '@/components/ui/label'
import { Select, SelectContent, SelectItem, SelectTrigger, SelectValue } from '@/components/ui/select'
import { Badge } from '@/components/ui/badge'
import { Alert, AlertDescription } from '@/components/ui/alert'
import { Play, AlertCircle, Loader2 } from 'lucide-react'
import { useStreamStore } from '@/stores/useStreamStore'
import { api } from '@/lib/api'
import type { DifferentialTestResult, DifferentialTestPoint } from '@/lib/types'

export default function DifferentialTestPage() {
  const { streams, fetchStreams } = useStreamStore()
  const [side1StreamId, setSide1StreamId] = useState<string>('')
  const [side2StreamId, setSide2StreamId] = useState<string>('')
  const [slope1, setSlope1] = useState('25')
  const [slope2, setSlope2] = useState('50')
  const [breakpoint, setBreakpoint] = useState('3.0')
  const [minPickup, setMinPickup] = useState('0.3')
  const [testPoints] = useState<DifferentialTestPoint[]>([
    { Ir: 1.0, Id: 0.5 },
    { Ir: 2.0, Id: 0.8 },
    { Ir: 5.0, Id: 1.5 },
  ])
  const [results, setResults] = useState<DifferentialTestResult[]>([])
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
    if (!side1StreamId || !side2StreamId) {
      setError('Please select both Side 1 and Side 2 streams')
      return
    }

    if (side1StreamId === side2StreamId) {
      setError('Side 1 and Side 2 must be different streams')
      return
    }

    setIsRunning(true)
    setError('')
    setResults([])

    try {
      const testResults = await api.runDifferentialTest({
        side1: side1StreamId,
        side2: side2StreamId,
        points: testPoints,
        settings: {
          slope1: parseFloat(slope1),
          slope2: parseFloat(slope2),
          breakpoint: parseFloat(breakpoint),
          minPickup: parseFloat(minPickup),
        },
      })

      setResults(testResults)
    } catch (err) {
      setError(err instanceof Error ? err.message : 'Failed to run differential test')
    } finally {
      setIsRunning(false)
    }
  }

  return (
    <div className="space-y-6">
      <div>
        <h1 className="text-3xl font-bold tracking-tight">Differential 87 Test</h1>
        <p className="text-muted-foreground">
          Automated testing for differential protection relays
        </p>
      </div>

      {error && (
        <Alert variant="destructive">
          <AlertCircle className="h-4 w-4" />
          <AlertDescription>{error}</AlertDescription>
        </Alert>
      )}

      <div className="grid gap-6 lg:grid-cols-2">
        <Card>
          <CardHeader><CardTitle>Test Configuration</CardTitle><CardDescription>Define restraint and differential current points for testing</CardDescription></CardHeader>
          <CardContent className="space-y-4">
            <div className="grid gap-4 sm:grid-cols-2">
              <div className="space-y-2">
                <Label htmlFor="side1">Side 1 Stream</Label>
                <Select value={side1StreamId} onValueChange={setSide1StreamId}>
                  <SelectTrigger id="side1"><SelectValue placeholder="Select stream" /></SelectTrigger>
                  <SelectContent>{streams.map((s) => <SelectItem key={s.id} value={s.id}>{s.name}</SelectItem>)}</SelectContent>
                </Select>
              </div>
              <div className="space-y-2">
                <Label htmlFor="side2">Side 2 Stream</Label>
                <Select value={side2StreamId} onValueChange={setSide2StreamId}>
                  <SelectTrigger id="side2"><SelectValue placeholder="Select stream" /></SelectTrigger>
                  <SelectContent>{streams.map((s) => <SelectItem key={s.id} value={s.id}>{s.name}</SelectItem>)}</SelectContent>
                </Select>
              </div>
            </div>
            <div className="grid gap-4 sm:grid-cols-2">
              <div className="space-y-2">
                <Label htmlFor="slope1">Slope 1 (%)</Label>
                <Input id="slope1" type="number" step="1" value={slope1} onChange={(e) => setSlope1(e.target.value)} />
              </div>
              <div className="space-y-2">
                <Label htmlFor="slope2">Slope 2 (%)</Label>
                <Input id="slope2" type="number" step="1" value={slope2} onChange={(e) => setSlope2(e.target.value)} />
              </div>
            </div>
            <div className="grid gap-4 sm:grid-cols-2">
              <div className="space-y-2">
                <Label htmlFor="breakpoint">Breakpoint (A)</Label>
                <Input id="breakpoint" type="number" step="0.1" value={breakpoint} onChange={(e) => setBreakpoint(e.target.value)} />
              </div>
              <div className="space-y-2">
                <Label htmlFor="minPickup">Min Pickup (A)</Label>
                <Input id="minPickup" type="number" step="0.1" value={minPickup} onChange={(e) => setMinPickup(e.target.value)} />
              </div>
            </div>
            <Button onClick={handleRunTest} disabled={!side1StreamId || !side2StreamId || isRunning} className="w-full">
              {isRunning ? <><Loader2 className="mr-2 h-4 w-4 animate-spin" />Running Test...</> : <><Play className="mr-2 h-4 w-4" />Run Test</>}
            </Button>
            <div className="space-y-2">
              <Label>Test Points</Label>
              {results.length > 0 ? (
                results.map((result, idx) => (
                  <div key={idx} className="flex items-center justify-between p-2 border rounded text-sm">
                    <span className="font-mono">Ir = {result.point.Ir.toFixed(1)}A, Id = {result.point.Id.toFixed(1)}A</span>
                    <div className="flex items-center gap-2">
                      {result.tripTime && <span className="text-xs text-muted-foreground">{result.tripTime.toFixed(2)}ms</span>}
                      <Badge variant={result.pass ? 'default' : 'destructive'}>{result.pass ? 'PASS' : 'FAIL'}</Badge>
                    </div>
                  </div>
                ))
              ) : (
                testPoints.map((pt, idx) => (
                  <div key={idx} className="flex items-center justify-between p-2 border rounded text-sm">
                    <span className="font-mono">Ir = {pt.Ir.toFixed(1)}A, Id = {pt.Id.toFixed(1)}A</span>
                    <Badge variant="outline">Pending</Badge>
                  </div>
                ))
              )}
            </div>
          </CardContent>
        </Card>

        <Card>
          <CardHeader><CardTitle>Id vs Ir Plot</CardTitle><CardDescription>Operating characteristic</CardDescription></CardHeader>
          <CardContent>
            <div className="h-96 flex items-center justify-center border rounded-lg bg-muted/20">
              <p className="text-sm text-muted-foreground">Differential characteristic plot coming soon...</p>
            </div>
          </CardContent>
        </Card>
      </div>
    </div>
  )
}
