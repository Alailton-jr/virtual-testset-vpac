import { useState, useEffect } from 'react'
import { Card, CardContent, CardDescription, CardHeader, CardTitle } from '@/components/ui/card'
import { Select, SelectContent, SelectItem, SelectTrigger, SelectValue } from '@/components/ui/select'
import { Button } from '@/components/ui/button'
import { Label } from '@/components/ui/label'
import { Switch } from '@/components/ui/switch'
import { Progress } from '@/components/ui/progress'
import { Alert, AlertDescription } from '@/components/ui/alert'
import { Play, Square, Loader2, AlertCircle } from 'lucide-react'
import { ComtradeUploader } from '@/components/ComtradeUploader'
import { ChannelMapper } from '@/components/ChannelMapper'
import { useStreamStore } from '@/stores/useStreamStore'
import { api } from '@/lib/api'
import type { ComtradeMetadata, ComtradeMapping } from '@/lib/types'

export default function ComtradePlaybackPage() {
  const { streams, fetchStreams, isLoading: streamsLoading } = useStreamStore()
  const [selectedStreamId, setSelectedStreamId] = useState<string>('')
  const [file, setFile] = useState<File | null>(null)
  const [metadata, setMetadata] = useState<ComtradeMetadata | null>(null)
  const [mapping, setMapping] = useState<ComtradeMapping[]>([])
  const [loop, setLoop] = useState(false)
  const [isPlaying, setIsPlaying] = useState(false)
  const [progress, setProgress] = useState(0)
  const [error, setError] = useState<string>('')

  // Fetch streams on mount
  useEffect(() => {
    fetchStreams()
  }, [fetchStreams])

  // Auto-select first stream
  useEffect(() => {
    if (streams.length > 0 && !selectedStreamId) {
      setSelectedStreamId(streams[0].id)
    }
  }, [streams, selectedStreamId])

  // Clear error after 5 seconds
  useEffect(() => {
    if (error) {
      const timer = setTimeout(() => setError(''), 5000)
      return () => clearTimeout(timer)
    }
  }, [error])

  const handleUpload = (uploadedFile: File, uploadedMetadata: ComtradeMetadata) => {
    setFile(uploadedFile)
    setMetadata(uploadedMetadata)
    setMapping([]) // Clear previous mapping
    setError('')
  }

  const handleClear = () => {
    setFile(null)
    setMetadata(null)
    setMapping([])
    setIsPlaying(false)
    setProgress(0)
    setError('')
  }

  const handleStartPlayback = async () => {
    if (!selectedStreamId || !file || !metadata) {
      setError('Please select a stream and upload a file')
      return
    }

    if (mapping.length === 0) {
      setError('Please map at least one channel')
      return
    }

    setIsPlaying(true)
    setProgress(0)
    setError('')

    try {
      await api.startComtradePlayback({
        streamId: selectedStreamId,
        file,
        mapping,
        loop,
      })

      // Simulate progress (in real implementation, this would come from WebSocket)
      const progressInterval = setInterval(() => {
        setProgress(prev => {
          if (prev >= 100) {
            clearInterval(progressInterval)
            if (!loop) {
              setIsPlaying(false)
            }
            return loop ? 0 : 100
          }
          return prev + 1
        })
      }, 100)
    } catch (err) {
      setError(err instanceof Error ? err.message : 'Failed to start playback')
      setIsPlaying(false)
    }
  }

  const handleStopPlayback = async () => {
    if (!selectedStreamId) return

    try {
      await api.stopComtradePlayback(selectedStreamId)
      setIsPlaying(false)
      setProgress(0)
    } catch (err) {
      setError(err instanceof Error ? err.message : 'Failed to stop playback')
    }
  }

  const canStartPlayback = selectedStreamId && file && metadata && mapping.length > 0 && !isPlaying

  return (
    <div className="space-y-6">
      {/* Header */}
      <div>
        <h1 className="text-3xl font-bold tracking-tight">COMTRADE Playback</h1>
        <p className="text-muted-foreground">
          Upload and replay COMTRADE or CSV waveform files
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
          <CardTitle>Target Stream</CardTitle>
          <CardDescription>
            Select the SV stream to play the COMTRADE data into
          </CardDescription>
        </CardHeader>
        <CardContent>
          {streamsLoading ? (
            <div className="flex items-center gap-2 text-muted-foreground">
              <Loader2 className="h-4 w-4 animate-spin" />
              <span>Loading streams...</span>
            </div>
          ) : streams.length === 0 ? (
            <p className="text-sm text-muted-foreground">
              No streams available. Create a stream first.
            </p>
          ) : (
            <div className="space-y-2">
              <Label htmlFor="stream">Stream</Label>
              <Select value={selectedStreamId} onValueChange={setSelectedStreamId} disabled={isPlaying}>
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
            </div>
          )}
        </CardContent>
      </Card>

      {/* File Upload */}
      <ComtradeUploader
        onUpload={handleUpload}
        onClear={handleClear}
        disabled={isPlaying}
      />

      {/* Channel Mapping */}
      {metadata && (
        <ChannelMapper
          metadata={metadata}
          mapping={mapping}
          onChange={setMapping}
          disabled={isPlaying}
        />
      )}

      {/* Playback Controls */}
      {file && metadata && (
        <Card>
          <CardHeader>
            <CardTitle>Playback Controls</CardTitle>
            <CardDescription>
              Start or stop COMTRADE playback
            </CardDescription>
          </CardHeader>
          <CardContent className="space-y-4">
            {/* Loop Option */}
            <div className="flex items-center justify-between">
              <div className="space-y-0.5">
                <Label htmlFor="loop">Loop Playback</Label>
                <p className="text-sm text-muted-foreground">
                  Continuously repeat the waveform
                </p>
              </div>
              <Switch
                id="loop"
                checked={loop}
                onCheckedChange={setLoop}
                disabled={isPlaying}
              />
            </div>

            {/* Progress Bar */}
            {isPlaying && (
              <div className="space-y-2">
                <div className="flex items-center justify-between text-sm">
                  <span className="text-muted-foreground">Progress</span>
                  <span className="font-medium">{progress}%</span>
                </div>
                <Progress value={progress} />
              </div>
            )}

            {/* Action Buttons */}
            <div className="flex gap-2">
              {!isPlaying ? (
                <Button
                  onClick={handleStartPlayback}
                  disabled={!canStartPlayback}
                  className="flex-1"
                >
                  <Play className="mr-2 h-4 w-4" />
                  Start Playback
                </Button>
              ) : (
                <Button
                  onClick={handleStopPlayback}
                  variant="destructive"
                  className="flex-1"
                >
                  <Square className="mr-2 h-4 w-4" />
                  Stop Playback
                </Button>
              )}
            </div>

            {/* Info */}
            {metadata && (
              <div className="text-sm text-muted-foreground space-y-1">
                <p>
                  • Duration: ~
                  {metadata.totalSamples > 0
                    ? ((metadata.totalSamples / metadata.sampleRate) * 1000).toFixed(0)
                    : '?'}{' '}
                  ms
                </p>
                <p>
                  • {mapping.length} channel{mapping.length !== 1 ? 's' : ''} mapped
                </p>
                {loop && <p>• Playback will loop continuously</p>}
              </div>
            )}
          </CardContent>
        </Card>
      )}
    </div>
  )
}
