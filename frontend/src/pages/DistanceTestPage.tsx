import { useState, useEffect } from 'react'
import { Card, CardContent, CardDescription, CardHeader, CardTitle } from '@/components/ui/card'
import { Button } from '@/components/ui/button'
import { Input } from '@/components/ui/input'
import { Label } from '@/components/ui/label'
import { Badge } from '@/components/ui/badge'
import { Select, SelectContent, SelectItem, SelectTrigger, SelectValue } from '@/components/ui/select'
import { Alert, AlertDescription } from '@/components/ui/alert'
import { Play, Loader2, AlertCircle, Trash2 } from 'lucide-react'
import { useStreamStore } from '@/stores/useStreamStore'
import { api } from '@/lib/api'
import type { DistanceTestPoint, DistanceTestResult } from '@/lib/types'

interface TestPointWithId extends DistanceTestPoint {
  id: number
}

export default function DistanceTestPage() {
  const { streams, fetchStreams } = useStreamStore()
  const [selectedStreamId, setSelectedStreamId] = useState<string>('')
  const [testPoints, setTestPoints] = useState<TestPointWithId[]>([
    { id: 1, R: 2.0, X: 4.0, faultType: 'AG' },
    { id: 2, R: 5.0, X: 10.0, faultType: 'ABC' },
  ])
  const [newR, setNewR] = useState('0')
  const [newX, setNewX] = useState('0')
  const [newFaultType, setNewFaultType] = useState<'AG' | 'BG' | 'CG' | 'AB' | 'BC' | 'CA' | 'ABC'>('AG')
  const [results, setResults] = useState<DistanceTestResult[]>([])
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

  const handleAddPoint = () => {
    const r = parseFloat(newR)
    const x = parseFloat(newX)
    if (isNaN(r) || isNaN(x)) {
      setError('Please enter valid R and X values')
      return
    }
    const maxId = testPoints.length > 0 ? Math.max(...testPoints.map(p => p.id)) : 0
    setTestPoints([...testPoints, { id: maxId + 1, R: r, X: x, faultType: newFaultType }])
    setNewR('0')
    setNewX('0')
  }

  const handleRemovePoint = (id: number) => {
    setTestPoints(testPoints.filter(p => p.id !== id))
  }

  const handleRunTest = async () => {
    if (!selectedStreamId) {
      setError('Please select a stream')
      return
    }

    if (testPoints.length === 0) {
      setError('Please add at least one test point')
      return
    }

    setIsRunning(true)
    setError('')
    setResults([])

    try {
      const testResults = await api.runDistanceTest({
        streamId: selectedStreamId,
        source: {
          RS1: 0.5,
          XS1: 5.0,
          RS0: 1.0,
          XS0: 10.0,
          Vprefault: 115.47,
        },
        points: testPoints.map(({ id, ...point }) => point),
      })

      setResults(testResults)
    } catch (err) {
      setError(err instanceof Error ? err.message : 'Failed to run distance test')
    } finally {
      setIsRunning(false)
    }
  }

  return (
    <div className="space-y-6">
      <div>
        <h1 className="text-3xl font-bold tracking-tight">Distance 21 Test</h1>
        <p className="text-muted-foreground">
          Automated testing for distance protection relays
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
          <CardHeader><CardTitle>Test Points Configuration</CardTitle><CardDescription>Define R-X impedance points and fault types for testing</CardDescription></CardHeader>
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
                <Label htmlFor="r">Resistance (Ω)</Label>
                <Input id="r" type="number" step="0.1" value={newR} onChange={(e) => setNewR(e.target.value)} />
              </div>
              <div className="space-y-2">
                <Label htmlFor="x">Reactance (Ω)</Label>
                <Input id="x" type="number" step="0.1" value={newX} onChange={(e) => setNewX(e.target.value)} />
              </div>
              <div className="space-y-2">
                <Label htmlFor="faultType">Fault Type</Label>
                <Select value={newFaultType} onValueChange={(val) => setNewFaultType(val as typeof newFaultType)}>
                  <SelectTrigger id="faultType"><SelectValue /></SelectTrigger>
                  <SelectContent>
                    <SelectItem value="AG">A-G</SelectItem>
                    <SelectItem value="BG">B-G</SelectItem>
                    <SelectItem value="CG">C-G</SelectItem>
                    <SelectItem value="AB">A-B</SelectItem>
                    <SelectItem value="BC">B-C</SelectItem>
                    <SelectItem value="CA">C-A</SelectItem>
                    <SelectItem value="ABC">A-B-C</SelectItem>
                  </SelectContent>
                </Select>
              </div>
            </div>
            <Button onClick={handleAddPoint} className="w-full">Add Test Point</Button>
            <div className="space-y-2">
              <Label>Test Points</Label>
              {results.length > 0 ? (
                results.map((result, idx) => (
                  <div key={idx} className="flex items-center justify-between p-2 border rounded text-sm">
                    <span className="font-mono">R={result.point.R}Ω, X={result.point.X}Ω ({result.point.faultType})</span>
                    <div className="flex items-center gap-2">
                      {result.tripTime && <span className="text-xs text-muted-foreground">{result.tripTime.toFixed(2)}ms</span>}
                      <Badge variant={result.pass ? 'default' : 'destructive'}>{result.pass ? 'PASS' : 'FAIL'}</Badge>
                    </div>
                  </div>
                ))
              ) : (
                testPoints.map((pt) => (
                  <div key={pt.id} className="flex items-center justify-between p-2 border rounded text-sm">
                    <span className="font-mono">R={pt.R}Ω, X={pt.X}Ω ({pt.faultType})</span>
                    <div className="flex items-center gap-2">
                      <Badge variant="outline">Pending</Badge>
                      <Button variant="ghost" size="sm" onClick={() => handleRemovePoint(pt.id)}>
                        <Trash2 className="h-4 w-4" />
                      </Button>
                    </div>
                  </div>
                ))
              )}
            </div>
          </CardContent>
        </Card>

        <Card>
          <CardHeader><CardTitle>R-X Diagram</CardTitle><CardDescription>Impedance plane visualization</CardDescription></CardHeader>
          <CardContent>
            <div className="h-64 flex items-center justify-center border rounded-lg bg-muted/20">
              <p className="text-sm text-muted-foreground">R-X diagram with zones coming soon...</p>
            </div>
            <div className="mt-4">
              <Button onClick={handleRunTest} disabled={isRunning || !selectedStreamId} className="w-full">
                {isRunning ? <><Loader2 className="mr-2 h-4 w-4 animate-spin" />Running Test...</> : <><Play className="mr-2 h-4 w-4" />Run Test</>}
              </Button>
            </div>
          </CardContent>
        </Card>
      </div>
    </div>
  )
}
