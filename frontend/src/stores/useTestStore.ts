import { create } from 'zustand'
import type {
  ImpedanceConfig,
  RampingTestConfig,
  DistanceTestConfig,
  OvercurrentTestConfig,
  DifferentialTestConfig,
  TestResult,
  TestProgress,
} from '@/lib/types'
import { api } from '@/lib/api'
import { ws } from '@/lib/ws'

type TestType = 'impedance' | 'ramping' | 'distance' | 'overcurrent' | 'differential'

interface TestStore {
  // State
  activeTest: TestType | null
  activeStreamId: string | null
  isRunning: boolean
  results: TestResult[]
  progress: TestProgress | null
  error: string | null

  // Actions - Impedance
  applyImpedance: (config: ImpedanceConfig) => Promise<void>
  clearImpedance: (streamId: string) => Promise<void>

  // Actions - Ramping Test
  runRampingTest: (config: RampingTestConfig) => Promise<void>
  stopRampingTest: (streamId: string) => Promise<void>

  // Actions - Distance Test
  runDistanceTest: (config: DistanceTestConfig) => Promise<void>
  stopDistanceTest: (streamId: string) => Promise<void>

  // Actions - Overcurrent Test
  runOvercurrentTest: (config: OvercurrentTestConfig) => Promise<void>
  stopOvercurrentTest: (streamId: string) => Promise<void>

  // Actions - Differential Test
  runDifferentialTest: (config: DifferentialTestConfig) => Promise<void>
  stopDifferentialTest: (side1: string, side2: string) => Promise<void>

  // General actions
  clearResults: () => void
  clearError: () => void
  handleTestProgress: (progress: TestProgress) => void
  handleTestComplete: (result: TestResult) => void
}

export const useTestStore = create<TestStore>((set) => ({
  // Initial state
  activeTest: null,
  activeStreamId: null,
  isRunning: false,
  results: [],
  progress: null,
  error: null,

  // Impedance actions
  applyImpedance: async (config) => {
    set({ error: null, activeTest: 'impedance', activeStreamId: config.streamId })
    try {
      await api.applyImpedance(config)
    } catch (error) {
      const message = error instanceof Error ? error.message : 'Failed to apply impedance'
      set({ error: message, activeTest: null, activeStreamId: null })
      throw error
    }
  },

  clearImpedance: async (streamId) => {
    set({ error: null })
    try {
      await api.clearImpedance(streamId)
      set({ activeTest: null, activeStreamId: null })
    } catch (error) {
      const message = error instanceof Error ? error.message : 'Failed to clear impedance'
      set({ error: message })
      throw error
    }
  },

  // Ramping test actions
  runRampingTest: async (config) => {
    set({ error: null, activeTest: 'ramping', activeStreamId: config.streamId, isRunning: true, progress: null })
    try {
      await api.runRampingTest(config)
    } catch (error) {
      const message = error instanceof Error ? error.message : 'Failed to run ramping test'
      set({ error: message, isRunning: false, activeTest: null, activeStreamId: null })
      throw error
    }
  },

  stopRampingTest: async (streamId) => {
    set({ error: null })
    try {
      await api.stopRampingTest(streamId)
      set({ isRunning: false, activeTest: null, activeStreamId: null, progress: null })
    } catch (error) {
      const message = error instanceof Error ? error.message : 'Failed to stop ramping test'
      set({ error: message })
      throw error
    }
  },

  // Distance test actions
  runDistanceTest: async (config) => {
    set({ error: null, activeTest: 'distance', activeStreamId: config.streamId, isRunning: true, progress: null })
    try {
      await api.runDistanceTest(config)
    } catch (error) {
      const message = error instanceof Error ? error.message : 'Failed to run distance test'
      set({ error: message, isRunning: false, activeTest: null, activeStreamId: null })
      throw error
    }
  },

  stopDistanceTest: async (streamId) => {
    set({ error: null })
    try {
      await api.stopDistanceTest(streamId)
      set({ isRunning: false, activeTest: null, activeStreamId: null, progress: null })
    } catch (error) {
      const message = error instanceof Error ? error.message : 'Failed to stop distance test'
      set({ error: message })
      throw error
    }
  },

  // Overcurrent test actions
  runOvercurrentTest: async (config) => {
    set({ error: null, activeTest: 'overcurrent', activeStreamId: config.streamId, isRunning: true, progress: null })
    try {
      await api.runOvercurrentTest(config)
    } catch (error) {
      const message = error instanceof Error ? error.message : 'Failed to run overcurrent test'
      set({ error: message, isRunning: false, activeTest: null, activeStreamId: null })
      throw error
    }
  },

  stopOvercurrentTest: async (streamId) => {
    set({ error: null })
    try {
      await api.stopOvercurrentTest(streamId)
      set({ isRunning: false, activeTest: null, activeStreamId: null, progress: null })
    } catch (error) {
      const message = error instanceof Error ? error.message : 'Failed to stop overcurrent test'
      set({ error: message })
      throw error
    }
  },

  // Differential test actions
  runDifferentialTest: async (config) => {
    set({ error: null, activeTest: 'differential', activeStreamId: config.side1, isRunning: true, progress: null })
    try {
      await api.runDifferentialTest(config)
    } catch (error) {
      const message = error instanceof Error ? error.message : 'Failed to run differential test'
      set({ error: message, isRunning: false, activeTest: null, activeStreamId: null })
      throw error
    }
  },

  stopDifferentialTest: async (side1, side2) => {
    set({ error: null })
    try {
      await api.stopDifferentialTest(side1, side2)
      set({ isRunning: false, activeTest: null, activeStreamId: null, progress: null })
    } catch (error) {
      const message = error instanceof Error ? error.message : 'Failed to stop differential test'
      set({ error: message })
      throw error
    }
  },

  // General actions
  clearResults: () => set({ results: [], progress: null }),
  clearError: () => set({ error: null }),

  handleTestProgress: (progress) => set({ progress }),

  handleTestComplete: (result) =>
    set((state) => ({
      results: [...state.results, result],
      isRunning: false,
      activeTest: null,
      progress: null,
    })),
}))

// Subscribe to test events
ws.subscribe('test/progress', (event) => {
  useTestStore.getState().handleTestProgress(event.data as TestProgress)
})

ws.subscribe('test/complete', (event) => {
  useTestStore.getState().handleTestComplete(event.data as TestResult)
})
