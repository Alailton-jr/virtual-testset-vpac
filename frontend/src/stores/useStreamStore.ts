import { create } from 'zustand'
import type { Stream, StreamConfig } from '@/lib/types'
import { api } from '@/lib/api'

interface StreamStore {
  // State
  streams: Stream[]
  selectedStreamId: string | null
  isLoading: boolean
  error: string | null

  // Actions
  fetchStreams: () => Promise<void>
  createStream: (config: StreamConfig) => Promise<Stream>
  updateStream: (id: string, config: Partial<StreamConfig>) => Promise<void>
  deleteStream: (id: string) => Promise<void>
  startStream: (id: string) => Promise<void>
  stopStream: (id: string) => Promise<void>
  selectStream: (id: string | null) => void
  clearError: () => void
}

export const useStreamStore = create<StreamStore>((set) => ({
  // Initial state
  streams: [],
  selectedStreamId: null,
  isLoading: false,
  error: null,

  // Fetch all streams
  fetchStreams: async () => {
    set({ isLoading: true, error: null })
    try {
      const streams = await api.getStreams()
      set({ streams, isLoading: false })
    } catch (error) {
      const message = error instanceof Error ? error.message : 'Failed to fetch streams'
      set({ error: message, isLoading: false })
      throw error
    }
  },

  // Create new stream
  createStream: async (config) => {
    set({ isLoading: true, error: null })
    try {
      const stream = await api.createStream(config)
      set((state) => ({
        streams: [...state.streams, stream],
        isLoading: false,
      }))
      return stream
    } catch (error) {
      const message = error instanceof Error ? error.message : 'Failed to create stream'
      set({ error: message, isLoading: false })
      throw error
    }
  },

  // Update existing stream
  updateStream: async (id, config) => {
    set({ isLoading: true, error: null })
    try {
      const updatedStream = await api.updateStream(id, config)
      set((state) => ({
        streams: state.streams.map((s) =>
          s.id === id ? updatedStream : s
        ),
        isLoading: false,
      }))
    } catch (error) {
      const message = error instanceof Error ? error.message : 'Failed to update stream'
      set({ error: message, isLoading: false })
      throw error
    }
  },

  // Delete stream
  deleteStream: async (id) => {
    set({ isLoading: true, error: null })
    try {
      await api.deleteStream(id)
      set((state) => ({
        streams: state.streams.filter((s) => s.id !== id),
        selectedStreamId: state.selectedStreamId === id ? null : state.selectedStreamId,
        isLoading: false,
      }))
    } catch (error) {
      const message = error instanceof Error ? error.message : 'Failed to delete stream'
      set({ error: message, isLoading: false })
      throw error
    }
  },

  // Start stream
  startStream: async (id) => {
    set({ error: null })
    try {
      await api.startStream(id)
      set((state) => ({
        streams: state.streams.map((s) =>
          s.id === id ? { ...s, state: 'running' as const } : s
        ),
      }))
    } catch (error) {
      const message = error instanceof Error ? error.message : 'Failed to start stream'
      set({ error: message })
      throw error
    }
  },

  // Stop stream
  stopStream: async (id) => {
    set({ error: null })
    try {
      await api.stopStream(id)
      set((state) => ({
        streams: state.streams.map((s) =>
          s.id === id ? { ...s, state: 'stopped' as const } : s
        ),
      }))
    } catch (error) {
      const message = error instanceof Error ? error.message : 'Failed to stop stream'
      set({ error: message })
      throw error
    }
  },

  // Select stream for editing/viewing
  selectStream: (id) => set({ selectedStreamId: id }),

  // Clear error
  clearError: () => set({ error: null }),
}))
