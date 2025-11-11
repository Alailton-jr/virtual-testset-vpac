import { useState } from 'react'
import { Play, Square, Edit, Trash2, Radio } from 'lucide-react'
import { Card, CardContent, CardHeader, CardTitle } from '@/components/ui/card'
import { Button } from '@/components/ui/button'
import { Badge } from '@/components/ui/badge'
import {
  AlertDialog,
  AlertDialogAction,
  AlertDialogCancel,
  AlertDialogContent,
  AlertDialogDescription,
  AlertDialogFooter,
  AlertDialogHeader,
  AlertDialogTitle,
} from '@/components/ui/alert-dialog'
import type { Stream } from '@/lib/types'
import { cn } from '@/lib/utils'

interface StreamCardProps {
  stream: Stream
  onStart: (id: string) => Promise<void>
  onStop: (id: string) => Promise<void>
  onEdit: (stream: Stream) => void
  onDelete: (id: string) => Promise<void>
}

export function StreamCard({ stream, onStart, onStop, onEdit, onDelete }: StreamCardProps) {
  const [loading, setLoading] = useState(false)
  const [showDeleteDialog, setShowDeleteDialog] = useState(false)

  const isRunning = stream.status === 'running'

  const handleToggleStream = async () => {
    setLoading(true)
    try {
      if (isRunning) {
        await onStop(stream.id)
      } else {
        await onStart(stream.id)
      }
    } catch (error) {
      console.error('Failed to toggle stream:', error)
    } finally {
      setLoading(false)
    }
  }

  const handleDelete = async () => {
    setLoading(true)
    try {
      await onDelete(stream.id)
      setShowDeleteDialog(false)
    } catch (error) {
      console.error('Failed to delete stream:', error)
    } finally {
      setLoading(false)
    }
  }

  return (
    <>
      <Card className={cn(
        'transition-colors',
        isRunning && 'border-green-500 dark:border-green-600'
      )}>
        <CardHeader>
          <div className="flex items-start justify-between">
            <div className="flex items-center gap-3">
              <div className={cn(
                'rounded-full p-2',
                isRunning 
                  ? 'bg-green-100 dark:bg-green-900/30' 
                  : 'bg-gray-100 dark:bg-gray-800'
              )}>
                <Radio className={cn(
                  'h-5 w-5',
                  isRunning 
                    ? 'text-green-600 dark:text-green-400' 
                    : 'text-gray-600 dark:text-gray-400'
                )} />
              </div>
              <div>
                <CardTitle className="text-lg">{stream.name}</CardTitle>
                <div className="text-sm text-muted-foreground flex items-center gap-2 mt-1">
                  <span>{stream.svID}</span>
                  <Badge 
                    variant={isRunning ? 'default' : 'secondary'}
                    className={cn(
                      isRunning && 'bg-green-600 hover:bg-green-700'
                    )}
                  >
                    {isRunning ? 'Running' : 'Stopped'}
                  </Badge>
                </div>
              </div>
            </div>

            <div className="flex items-center gap-2">
              <Button
                size="sm"
                variant={isRunning ? 'destructive' : 'default'}
                onClick={handleToggleStream}
                disabled={loading}
              >
                {loading ? (
                  'Loading...'
                ) : isRunning ? (
                  <>
                    <Square className="h-4 w-4 mr-1" />
                    Stop
                  </>
                ) : (
                  <>
                    <Play className="h-4 w-4 mr-1" />
                    Start
                  </>
                )}
              </Button>
              <Button
                size="sm"
                variant="outline"
                onClick={() => onEdit(stream)}
                disabled={loading || isRunning}
              >
                <Edit className="h-4 w-4" />
              </Button>
              <Button
                size="sm"
                variant="outline"
                onClick={() => setShowDeleteDialog(true)}
                disabled={loading || isRunning}
              >
                <Trash2 className="h-4 w-4" />
              </Button>
            </div>
          </div>
        </CardHeader>

        <CardContent>
          <div className="grid grid-cols-2 md:grid-cols-3 gap-4 text-sm">
            <div>
              <p className="text-muted-foreground">App ID</p>
              <p className="font-mono">{stream.appIdHex}</p>
            </div>
            <div>
              <p className="text-muted-foreground">MAC Destination</p>
              <p className="font-mono text-xs">{stream.macDst}</p>
            </div>
            <div>
              <p className="text-muted-foreground">VLAN</p>
              <p>{stream.vlanId} (Priority: {stream.vlanPriority})</p>
            </div>
            <div>
              <p className="text-muted-foreground">Sample Rate</p>
              <p>{stream.smpRate} Hz</p>
            </div>
            <div>
              <p className="text-muted-foreground">Channels</p>
              <p>{stream.noChannels}</p>
            </div>
            <div>
              <p className="text-muted-foreground">Dataset</p>
              <p>{stream.datSet || 'N/A'}</p>
            </div>
          </div>
        </CardContent>
      </Card>

      <AlertDialog open={showDeleteDialog} onOpenChange={setShowDeleteDialog}>
        <AlertDialogContent>
          <AlertDialogHeader>
            <AlertDialogTitle>Delete Stream</AlertDialogTitle>
            <AlertDialogDescription>
              Are you sure you want to delete stream "{stream.name}"? This action cannot be undone.
            </AlertDialogDescription>
          </AlertDialogHeader>
          <AlertDialogFooter>
            <AlertDialogCancel disabled={loading}>Cancel</AlertDialogCancel>
            <AlertDialogAction
              onClick={handleDelete}
              disabled={loading}
              className="bg-red-600 hover:bg-red-700"
            >
              {loading ? 'Deleting...' : 'Delete'}
            </AlertDialogAction>
          </AlertDialogFooter>
        </AlertDialogContent>
      </AlertDialog>
    </>
  )
}
