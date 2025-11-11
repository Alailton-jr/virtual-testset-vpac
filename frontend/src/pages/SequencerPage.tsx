import { useState, useEffect } from 'react'
import { Card, CardContent, CardDescription, CardHeader, CardTitle } from '@/components/ui/card'
import { Button } from '@/components/ui/button'
import { Input } from '@/components/ui/input'
import { Label } from '@/components/ui/label'
import { Select, SelectContent, SelectItem, SelectTrigger, SelectValue } from '@/components/ui/select'
import { Alert, AlertDescription } from '@/components/ui/alert'
import { Play, Square, Plus, Trash2, AlertCircle } from 'lucide-react'
import { useStreamStore } from '@/stores/useStreamStore'
import { api } from '@/lib/api'
import type { Sequence, SequenceState } from '@/lib/types'

export default function SequencerPage() {
  const { streams, fetchStreams } = useStreamStore()
  const [savedSequences, setSavedSequences] = useState<Sequence[]>([])
  const [currentSequence, setCurrentSequence] = useState<Sequence | null>(null)
  const [sequenceName, setSequenceName] = useState('New Sequence')
  const [selectedStreamId, setSelectedStreamId] = useState<string>('')
  const [states, setStates] = useState<SequenceState[]>([
    { name: 'Pre-Fault', duration: 2000, phasors: {} }
  ])
  const [loopSequence, setLoopSequence] = useState(false)
  const [isRunning, setIsRunning] = useState(false)
  const [error, setError] = useState<string>('')

  useEffect(() => { fetchStreams(); loadSequences() }, [fetchStreams])
  useEffect(() => { if (error) { const timer = setTimeout(() => setError(''), 5000); return () => clearTimeout(timer) } }, [error])

  const loadSequences = async () => {
    try {
      const sequences = await api.getSequences()
      setSavedSequences(sequences)
    } catch (err) {
      setError(err instanceof Error ? err.message : 'Failed to load sequences')
    }
  }

  const handleSaveSequence = async () => {
    if (!selectedStreamId) { setError('Select a stream first'); return }
    if (states.length === 0) { setError('Add at least one state'); return }
    
    try {
      const sequenceData = {
        name: sequenceName,
        streamId: selectedStreamId,
        states,
        loop: loopSequence
      }

      if (currentSequence) {
        await api.updateSequence(currentSequence.id, sequenceData)
      } else {
        await api.createSequence(sequenceData)
      }
      
      await loadSequences()
      setError('')
    } catch (err) {
      setError(err instanceof Error ? err.message : 'Failed to save sequence')
    }
  }

  const handleLoadSequence = (sequence: Sequence) => {
    setCurrentSequence(sequence)
    setSequenceName(sequence.name)
    setSelectedStreamId(sequence.streamId)
    setStates(sequence.states)
    setLoopSequence(sequence.loop)
  }

  const handleDeleteSequence = async (id: string) => {
    try {
      await api.deleteSequence(id)
      await loadSequences()
      if (currentSequence?.id === id) {
        setCurrentSequence(null)
      }
    } catch (err) {
      setError(err instanceof Error ? err.message : 'Failed to delete sequence')
    }
  }

  const addState = () => { 
    setStates([...states, { 
      name: `State ${states.length + 1}`, 
      duration: 1000, 
      phasors: {} 
    }]) 
  }
  
  const removeState = (index: number) => { 
    if (states.length > 1) setStates(states.filter((_, i) => i !== index)) 
  }
  
  const updateState = (index: number, updates: Partial<SequenceState>) => { 
    setStates(states.map((state, i) => (i === index ? { ...state, ...updates } : state))) 
  }

  const handleStart = async () => {
    if (!currentSequence) { 
      setError('Save the sequence first before running')
      return 
    }
    
    setIsRunning(true)
    setError('')
    
    try {
      await api.runSequence({
        sequenceId: currentSequence.id,
        streamId: selectedStreamId,
        loop: loopSequence
      })
    } catch (err) { 
      setError(err instanceof Error ? err.message : 'Failed to start sequence')
      setIsRunning(false) 
    }
  }

  const handleStop = async () => {
    if (!selectedStreamId) return
    
    try { 
      await api.stopSequence(selectedStreamId)
      setIsRunning(false)
    } catch (err) { 
      setError(err instanceof Error ? err.message : 'Failed to stop sequence') 
    }
  }

  return (
    <div className="space-y-6">
      <div>
        <h1 className="text-3xl font-bold tracking-tight">Sequencer</h1>
        <p className="text-muted-foreground">Create and execute automated test sequences</p>
      </div>

      {error && <Alert variant="destructive"><AlertCircle className="h-4 w-4" /><AlertDescription>{error}</AlertDescription></Alert>}

      <div className="grid grid-cols-3 gap-6">
        {/* Saved Sequences Sidebar */}
        <Card className="col-span-1">
          <CardHeader>
            <CardTitle>Saved Sequences</CardTitle>
            <CardDescription>Load existing sequences</CardDescription>
          </CardHeader>
          <CardContent className="space-y-2">
            {savedSequences.length === 0 ? (
              <p className="text-sm text-muted-foreground">No saved sequences</p>
            ) : (
              savedSequences.map(seq => (
                <div key={seq.id} className="flex items-center justify-between p-2 border rounded hover:bg-muted">
                  <button
                    onClick={() => handleLoadSequence(seq)}
                    className="flex-1 text-left text-sm font-medium"
                    disabled={isRunning}
                  >
                    {seq.name}
                  </button>
                  <Button
                    variant="ghost"
                    size="sm"
                    onClick={() => handleDeleteSequence(seq.id)}
                    disabled={isRunning}
                  >
                    <Trash2 className="h-4 w-4" />
                  </Button>
                </div>
              ))
            )}
          </CardContent>
        </Card>

        {/* Main Editor */}
        <div className="col-span-2 space-y-6">
          <Card>
            <CardHeader>
              <CardTitle>Sequence Configuration</CardTitle>
              <CardDescription>Define sequence name, stream, and settings</CardDescription>
            </CardHeader>
            <CardContent className="space-y-4">
              <div className="grid grid-cols-2 gap-4">
                <div className="space-y-2">
                  <Label>Sequence Name</Label>
                  <Input 
                    value={sequenceName} 
                    onChange={(e) => setSequenceName(e.target.value)}
                    disabled={isRunning}
                    placeholder="Enter sequence name"
                  />
                </div>
                <div className="space-y-2">
                  <Label>Target Stream</Label>
                  <Select value={selectedStreamId} onValueChange={setSelectedStreamId} disabled={isRunning}>
                    <SelectTrigger>
                      <SelectValue placeholder="Select stream" />
                    </SelectTrigger>
                    <SelectContent>
                      {streams.map(stream => (
                        <SelectItem key={stream.id} value={stream.id}>
                          {stream.name}
                        </SelectItem>
                      ))}
                    </SelectContent>
                  </Select>
                </div>
              </div>
              <div className="flex items-center gap-2">
                <input
                  type="checkbox"
                  checked={loopSequence}
                  onChange={(e) => setLoopSequence(e.target.checked)}
                  disabled={isRunning}
                  className="rounded"
                />
                <Label>Loop sequence continuously</Label>
              </div>
              <Button onClick={handleSaveSequence} disabled={isRunning} className="w-full">
                Save Sequence
              </Button>
            </CardContent>
          </Card>

          <div className="space-y-4">
            <div className="flex items-center justify-between">
              <h2 className="text-xl font-semibold">Sequence States</h2>
              <Button onClick={addState} disabled={isRunning} size="sm">
                <Plus className="mr-2 h-4 w-4" />Add State
              </Button>
            </div>

            {states.map((state, index) => (
              <Card key={index}>
                <CardHeader>
                  <div className="flex items-center justify-between">
                    <CardTitle className="text-base">State {index + 1}: {state.name}</CardTitle>
                    <Button 
                      variant="ghost" 
                      size="sm" 
                      onClick={() => removeState(index)} 
                      disabled={isRunning || states.length === 1}
                    >
                      <Trash2 className="h-4 w-4" />
                    </Button>
                  </div>
                </CardHeader>
                <CardContent>
                  <div className="grid grid-cols-2 gap-4">
                    <div className="space-y-2">
                      <Label>State Name</Label>
                      <Input 
                        value={state.name} 
                        onChange={(e) => updateState(index, { name: e.target.value })} 
                        disabled={isRunning} 
                      />
                    </div>
                    <div className="space-y-2">
                      <Label>Duration (milliseconds)</Label>
                      <Input 
                        type="number" 
                        value={state.duration} 
                        onChange={(e) => updateState(index, { duration: parseInt(e.target.value) || 0 })} 
                        disabled={isRunning} 
                        step={100}
                      />
                    </div>
                  </div>
                  <div className="mt-4 p-3 bg-muted/20 rounded-md">
                    <p className="text-sm text-muted-foreground">
                      Phasor configuration for this state would be defined here in full implementation
                    </p>
                  </div>
                </CardContent>
              </Card>
            ))}
          </div>

          <Card>
            <CardHeader>
              <CardTitle>Sequence Control</CardTitle>
            </CardHeader>
            <CardContent className="space-y-4">
              <div className="flex gap-2">
                {!isRunning ? (
                  <Button 
                    onClick={handleStart} 
                    disabled={!currentSequence || !selectedStreamId} 
                    className="flex-1"
                  >
                    <Play className="mr-2 h-4 w-4" />Start Sequence
                  </Button>
                ) : (
                  <Button onClick={handleStop} variant="destructive" className="flex-1">
                    <Square className="mr-2 h-4 w-4" />Stop Sequence
                  </Button>
                )}
              </div>
              <div className="text-sm text-muted-foreground space-y-1">
                <p>• {states.length} state(s)</p>
                <p>• Total Duration: {(states.reduce((sum, s) => sum + s.duration, 0) / 1000).toFixed(1)}s</p>
                {loopSequence && <p>• Loop: Enabled</p>}
              </div>
            </CardContent>
          </Card>
        </div>
      </div>
    </div>
  )
}
