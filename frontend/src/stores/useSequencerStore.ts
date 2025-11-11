import { create } from 'zustand'
import type { Sequence, SequenceState, SequenceRun } from '@/lib/types'
import { api } from '@/lib/api'
import { ws } from '@/lib/ws'

interface SequencerStore {
  // State
  sequences: Sequence[]
  currentSequenceId: string | null
  isRunning: boolean
  currentRun: SequenceRun | null
  isLoading: boolean
  error: string | null

  // Actions
  fetchSequences: () => Promise<void>
  createSequence: (sequence: Omit<Sequence, 'id'>) => Promise<Sequence>
  updateSequence: (id: string, sequence: Partial<Sequence>) => Promise<void>
  deleteSequence: (id: string) => Promise<void>
  runSequence: (id: string) => Promise<void>
  stopSequence: () => Promise<void>
  selectSequence: (id: string | null) => void
  clearError: () => void
  handleSequenceEvent: (state: SequenceState) => void
}

export const useSequencerStore = create<SequencerStore>((set) => ({
  // Initial state
  sequences: [],
  currentSequenceId: null,
  isRunning: false,
  currentRun: null,
  isLoading: false,
  error: null,

  // Fetch all sequences
  fetchSequences: async () => {
    set({ isLoading: true, error: null })
    try {
      const sequences = await api.getSequences()
      set({ sequences, isLoading: false })
    } catch (error) {
      const message = error instanceof Error ? error.message : 'Failed to fetch sequences'
      set({ error: message, isLoading: false })
      throw error
    }
  },

  // Create new sequence
  createSequence: async (sequence) => {
    set({ isLoading: true, error: null })
    try {
      const newSequence = await api.createSequence(sequence)
      set((state) => ({
        sequences: [...state.sequences, newSequence],
        isLoading: false,
      }))
      return newSequence
    } catch (error) {
      const message = error instanceof Error ? error.message : 'Failed to create sequence'
      set({ error: message, isLoading: false })
      throw error
    }
  },

  // Update existing sequence
  updateSequence: async (id, sequence) => {
    set({ isLoading: true, error: null })
    try {
      const updatedSequence = await api.updateSequence(id, sequence)
      set((state) => ({
        sequences: state.sequences.map((s) =>
          s.id === id ? updatedSequence : s
        ),
        isLoading: false,
      }))
    } catch (error) {
      const message = error instanceof Error ? error.message : 'Failed to update sequence'
      set({ error: message, isLoading: false })
      throw error
    }
  },

  // Delete sequence
  deleteSequence: async (id) => {
    set({ isLoading: true, error: null })
    try {
      await api.deleteSequence(id)
      set((state) => ({
        sequences: state.sequences.filter((s) => s.id !== id),
        currentSequenceId: state.currentSequenceId === id ? null : state.currentSequenceId,
        isLoading: false,
      }))
    } catch (error) {
      const message = error instanceof Error ? error.message : 'Failed to delete sequence'
      set({ error: message, isLoading: false })
      throw error
    }
  },

  // Run sequence
  runSequence: async (id) => {
    set({ error: null })
    try {
      const sequence = useSequencerStore.getState().sequences.find(s => s.id === id)
      if (!sequence) {
        throw new Error('Sequence not found')
      }
      await api.runSequence({
        sequenceId: id,
        streamId: sequence.streamId,
        loop: sequence.loop,
      })
      set({
        currentSequenceId: id,
        isRunning: true,
      })
    } catch (error) {
      const message = error instanceof Error ? error.message : 'Failed to run sequence'
      set({ error: message })
      throw error
    }
  },

  // Stop sequence
  stopSequence: async () => {
    set({ error: null })
    try {
      const currentSequenceId = useSequencerStore.getState().currentSequenceId
      if (!currentSequenceId) {
        throw new Error('No sequence is running')
      }
      const sequence = useSequencerStore.getState().sequences.find(s => s.id === currentSequenceId)
      if (!sequence) {
        throw new Error('Sequence not found')
      }
      await api.stopSequence(sequence.streamId)
      set({
        isRunning: false,
        currentRun: null,
      })
    } catch (error) {
      const message = error instanceof Error ? error.message : 'Failed to stop sequence'
      set({ error: message })
      throw error
    }
  },

  // Select sequence for editing/viewing
  selectSequence: (id) => set({ currentSequenceId: id }),

  // Clear error
  clearError: () => set({ error: null }),

  // Handle WebSocket sequence events
  handleSequenceEvent: (state) => {
    set({
      currentRun: state as unknown as SequenceRun | null,
      isRunning: !!(state as unknown as SequenceRun),
    })
  },
}))

// Subscribe to sequence events
ws.subscribe('sequencer/state', (event) => {
  useSequencerStore.getState().handleSequenceEvent(event.data as SequenceState)
})
