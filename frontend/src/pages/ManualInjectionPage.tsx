import { useState, useEffect } from 'react'
import { Card, CardContent, CardDescription, CardHeader, CardTitle } from '@/components/ui/card'
import { Select, SelectContent, SelectItem, SelectTrigger, SelectValue } from '@/components/ui/select'
import { Tabs, TabsContent, TabsList, TabsTrigger } from '@/components/ui/tabs'
import { Alert, AlertDescription } from '@/components/ui/alert'
import { Label } from '@/components/ui/label'
import { Loader2, AlertCircle, Radio } from 'lucide-react'
import { PhasorControl, type PhasorValues } from '@/components/PhasorControl'
import { HarmonicsEditor, type HarmonicsValues } from '@/components/HarmonicsEditor'
import { useStreamStore } from '@/stores/useStreamStore'
import { api } from '@/lib/api'

export default function ManualInjectionPage() {
  const { streams, fetchStreams, isLoading: streamsLoading } = useStreamStore()
  const [selectedStreamId, setSelectedStreamId] = useState<string>('')
  const [error, setError] = useState<string>('')
  const [activeTab, setActiveTab] = useState('phasors')

  // Fetch streams on mount
  useEffect(() => {
    fetchStreams()
  }, [fetchStreams])

  // Auto-select first running stream
  useEffect(() => {
    if (streams.length > 0 && !selectedStreamId) {
      const runningStream = streams.find(s => s.status === 'running')
      const firstStream = runningStream || streams[0]
      setSelectedStreamId(firstStream.id)
    }
  }, [streams, selectedStreamId])

  // Clear error after 5 seconds
  useEffect(() => {
    if (error) {
      const timer = setTimeout(() => setError(''), 5000)
      return () => clearTimeout(timer)
    }
  }, [error])

  const handlePhasorApply = async (values: PhasorValues) => {
    if (!selectedStreamId) {
      setError('Please select a stream')
      return
    }

    try {
      await api.updatePhasors(selectedStreamId, values)
      setError('')
    } catch (err) {
      setError(err instanceof Error ? err.message : 'Failed to update phasors')
      throw err
    }
  }

  const handleHarmonicsApply = async (values: HarmonicsValues) => {
    if (!selectedStreamId) {
      setError('Please select a stream')
      return
    }

    try {
      await api.updateHarmonics(selectedStreamId, values)
      setError('')
    } catch (err) {
      setError(err instanceof Error ? err.message : 'Failed to update harmonics')
      throw err
    }
  }

  const selectedStream = streams.find(s => s.id === selectedStreamId)
  const isStreamRunning = selectedStream?.status === 'running'

  return (
    <div className="space-y-6">
      {/* Header */}
      <div>
        <h1 className="text-3xl font-bold tracking-tight">Manual Injection</h1>
        <p className="text-muted-foreground">
          Manually inject phasor values and harmonics into SV streams
        </p>
      </div>

      {/* Error Alert */}
      {error && (
        <Alert variant="destructive">
          <AlertCircle className="h-4 w-4" />
          <AlertDescription>{error}</AlertDescription>
        </Alert>
      )}

      {/* Stream Selection */}
      <Card>
        <CardHeader>
          <CardTitle>Stream Selection</CardTitle>
          <CardDescription>
            Select a stream to inject phasor and harmonic data
          </CardDescription>
        </CardHeader>
        <CardContent>
          {streamsLoading ? (
            <div className="flex items-center gap-2 text-muted-foreground">
              <Loader2 className="h-4 w-4 animate-spin" />
              <span>Loading streams...</span>
            </div>
          ) : streams.length === 0 ? (
            <div className="text-center py-6">
              <Radio className="mx-auto h-12 w-12 text-muted-foreground/50" />
              <p className="mt-2 text-sm text-muted-foreground">
                No streams available. Create a stream first.
              </p>
            </div>
          ) : (
            <div className="space-y-2">
              <Label htmlFor="stream">Active Stream</Label>
              <Select value={selectedStreamId} onValueChange={setSelectedStreamId}>
                <SelectTrigger id="stream">
                  <SelectValue placeholder="Select a stream" />
                </SelectTrigger>
                <SelectContent>
                  {streams.map(stream => (
                    <SelectItem key={stream.id} value={stream.id}>
                      {stream.name} ({stream.svID}) - {stream.status}
                    </SelectItem>
                  ))}
                </SelectContent>
              </Select>
              {selectedStream && !isStreamRunning && (
                <p className="text-sm text-amber-600">
                  ⚠️ Stream is not running. Start the stream to see injected values.
                </p>
              )}
            </div>
          )}
        </CardContent>
      </Card>

      {/* Injection Controls */}
      {selectedStreamId && (
        <Tabs value={activeTab} onValueChange={setActiveTab}>
          <TabsList className="grid w-full grid-cols-2">
            <TabsTrigger value="phasors">Phasor Injection</TabsTrigger>
            <TabsTrigger value="harmonics">Harmonics</TabsTrigger>
          </TabsList>

          <TabsContent value="phasors" className="mt-6">
            <PhasorControl
              onApply={handlePhasorApply}
              disabled={!selectedStreamId}
            />
          </TabsContent>

          <TabsContent value="harmonics" className="mt-6">
            <HarmonicsEditor
              onApply={handleHarmonicsApply}
              disabled={!selectedStreamId}
            />
          </TabsContent>
        </Tabs>
      )}
    </div>
  )
}

