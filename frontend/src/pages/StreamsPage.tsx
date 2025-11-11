import { useEffect, useState } from 'react'
import { Plus, AlertCircle, Radio } from 'lucide-react'
import { Card, CardContent } from '@/components/ui/card'
import { Button } from '@/components/ui/button'
import { Alert, AlertDescription, AlertTitle } from '@/components/ui/alert'
import { StreamCard } from '@/components/StreamCard'
import { StreamConfigDialog } from '@/components/StreamConfigDialog'
import { useStreamStore } from '@/stores/useStreamStore'
import type { Stream, StreamConfig } from '@/lib/types'

export default function StreamsPage() {
  const {
    streams,
    fetchStreams,
    createStream,
    updateStream,
    deleteStream,
    startStream,
    stopStream,
    isLoading,
    error,
    clearError,
  } = useStreamStore()

  const [dialogOpen, setDialogOpen] = useState(false)
  const [editingStream, setEditingStream] = useState<Stream | null>(null)

  // Fetch streams on mount
  useEffect(() => {
    fetchStreams()
  }, [fetchStreams])

  // Clear error after 5 seconds
  useEffect(() => {
    if (error) {
      const timer = setTimeout(() => {
        clearError()
      }, 5000)
      return () => clearTimeout(timer)
    }
  }, [error, clearError])

  const handleCreate = () => {
    setEditingStream(null)
    setDialogOpen(true)
  }

  const handleEdit = (stream: Stream) => {
    setEditingStream(stream)
    setDialogOpen(true)
  }

  const handleSave = async (config: StreamConfig) => {
    if (editingStream) {
      await updateStream(editingStream.id, config)
    } else {
      await createStream(config)
    }
  }

  const handleStart = async (id: string) => {
    await startStream(id)
  }

  const handleStop = async (id: string) => {
    await stopStream(id)
  }

  const handleDelete = async (id: string) => {
    await deleteStream(id)
  }

  return (
    <div className="space-y-6">
      {/* Header */}
      <div className="flex items-center justify-between">
        <div>
          <h1 className="text-3xl font-bold tracking-tight">SV Streams</h1>
          <p className="text-muted-foreground">
            Manage Sampled Values (SV) streams for IEC 61850-9-2
          </p>
        </div>
        <Button onClick={handleCreate} disabled={isLoading}>
          <Plus className="h-4 w-4 mr-2" />
          Create Stream
        </Button>
      </div>

      {/* Error Alert */}
      {error && (
        <Alert variant="destructive">
          <AlertCircle className="h-4 w-4" />
          <AlertTitle>Error</AlertTitle>
          <AlertDescription>{error}</AlertDescription>
        </Alert>
      )}

      {/* Loading State */}
      {isLoading && streams.length === 0 && (
        <Card>
          <CardContent className="flex items-center justify-center py-12">
            <div className="text-center">
              <div className="animate-spin rounded-full h-12 w-12 border-b-2 border-primary mx-auto mb-4"></div>
              <p className="text-muted-foreground">Loading streams...</p>
            </div>
          </CardContent>
        </Card>
      )}

      {/* Empty State */}
      {!isLoading && streams.length === 0 && (
        <Card>
          <CardContent className="flex items-center justify-center py-12">
            <div className="text-center">
              <Radio className="h-16 w-16 text-muted-foreground mx-auto mb-4" />
              <h3 className="text-lg font-semibold mb-2">No streams configured</h3>
              <p className="text-muted-foreground mb-4">
                Create your first SV stream to start publishing sampled values
              </p>
              <Button onClick={handleCreate}>
                <Plus className="h-4 w-4 mr-2" />
                Create First Stream
              </Button>
            </div>
          </CardContent>
        </Card>
      )}

      {/* Stream List */}
      {streams.length > 0 && (
        <div className="grid gap-4">
          {streams.map((stream) => (
            <StreamCard
              key={stream.id}
              stream={stream}
              onStart={handleStart}
              onStop={handleStop}
              onEdit={handleEdit}
              onDelete={handleDelete}
            />
          ))}
        </div>
      )}

      {/* Stream Configuration Dialog */}
      <StreamConfigDialog
        open={dialogOpen}
        onOpenChange={setDialogOpen}
        stream={editingStream}
        onSave={handleSave}
      />
    </div>
  )
}

