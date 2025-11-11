import { create } from 'zustand'
import type {
  WaveformData,
  PhasorTableRow,
  VectorDiagramPoint,
  HarmonicsData,
} from '@/lib/types'
import { api } from '@/lib/api'
import { ws } from '@/lib/ws'

interface AnalyzerStore {
  // State
  streamId: string | null
  isAnalyzing: boolean
  waveformData: WaveformData | null
  phasorData: PhasorTableRow[]
  vectorData: VectorDiagramPoint[]
  harmonicsData: HarmonicsData | null
  error: string | null

  // Actions
  startAnalyzer: (streamId: string) => Promise<void>
  stopAnalyzer: () => Promise<void>
  clearData: () => void
  clearError: () => void
  handleWaveformUpdate: (data: WaveformData) => void
  handlePhasorUpdate: (data: PhasorTableRow[]) => void
  handleVectorUpdate: (data: VectorDiagramPoint[]) => void
  handleHarmonicsUpdate: (data: HarmonicsData) => void
}

export const useAnalyzerStore = create<AnalyzerStore>((set) => ({
  // Initial state
  streamId: null,
  isAnalyzing: false,
  waveformData: null,
  phasorData: [],
  vectorData: [],
  harmonicsData: null,
  error: null,

  // Start analyzer
  startAnalyzer: async (streamId) => {
    set({ error: null })
    try {
      await api.startAnalyzer(streamId)
      set({ isAnalyzing: true, streamId })
    } catch (error) {
      const message = error instanceof Error ? error.message : 'Failed to start analyzer'
      set({ error: message })
      throw error
    }
  },

  // Stop analyzer
  stopAnalyzer: async () => {
    set({ error: null })
    try {
      await api.stopAnalyzer()
      set({ isAnalyzing: false, streamId: null })
    } catch (error) {
      const message = error instanceof Error ? error.message : 'Failed to stop analyzer'
      set({ error: message })
      throw error
    }
  },

  // Clear all data
  clearData: () =>
    set({
      waveformData: null,
      phasorData: [],
      vectorData: [],
      harmonicsData: null,
    }),

  // Clear error
  clearError: () => set({ error: null }),

  // Handle WebSocket updates
  handleWaveformUpdate: (data) => set({ waveformData: data }),
  handlePhasorUpdate: (data) => set({ phasorData: data }),
  handleVectorUpdate: (data) => set({ vectorData: data }),
  handleHarmonicsUpdate: (data) => set({ harmonicsData: data }),
}))

// Subscribe to analyzer events
ws.subscribe('analyzer/waveform', (event) => {
  useAnalyzerStore.getState().handleWaveformUpdate(event.data as WaveformData)
})

ws.subscribe('analyzer/phasors', (event) => {
  useAnalyzerStore.getState().handlePhasorUpdate(event.data as PhasorTableRow[])
})

ws.subscribe('analyzer/vector', (event) => {
  useAnalyzerStore.getState().handleVectorUpdate(event.data as VectorDiagramPoint[])
})

ws.subscribe('analyzer/harmonics', (event) => {
  useAnalyzerStore.getState().handleHarmonicsUpdate(event.data as HarmonicsData)
})
