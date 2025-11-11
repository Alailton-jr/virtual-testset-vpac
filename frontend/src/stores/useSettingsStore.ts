import { create } from 'zustand'
import { persist } from 'zustand/middleware'

export type Theme = 'light' | 'dark' | 'system'
export type FrequencyUnit = 'hz' | 'samples'

interface Settings {
  // Backend connection
  backendUrl: string
  wsUrl: string
  
  // UI preferences
  theme: Theme
  frequencyUnit: FrequencyUnit
  autoRefresh: boolean
  refreshInterval: number // in seconds
  
  // Analyzer preferences
  maxWaveformPoints: number
  phasorDiagramScale: number
  
  // Notifications
  showNotifications: boolean
  notificationDuration: number // in seconds
}

interface SettingsStore extends Settings {
  // Actions
  updateSettings: (settings: Partial<Settings>) => void
  resetSettings: () => void
  setTheme: (theme: Theme) => void
  setFrequencyUnit: (unit: FrequencyUnit) => void
  toggleAutoRefresh: () => void
}

const DEFAULT_SETTINGS: Settings = {
  backendUrl: '/api/v1',
  wsUrl: 'ws://localhost:8080/ws',
  theme: 'system',
  frequencyUnit: 'hz',
  autoRefresh: false,
  refreshInterval: 5,
  maxWaveformPoints: 1000,
  phasorDiagramScale: 1.2,
  showNotifications: true,
  notificationDuration: 5,
}

export const useSettingsStore = create<SettingsStore>()(
  persist(
    (set) => ({
      ...DEFAULT_SETTINGS,

      updateSettings: (settings) =>
        set((state) => ({
          ...state,
          ...settings,
        })),

      resetSettings: () => set(DEFAULT_SETTINGS),

      setTheme: (theme) => set({ theme }),

      setFrequencyUnit: (unit) => set({ frequencyUnit: unit }),

      toggleAutoRefresh: () =>
        set((state) => ({ autoRefresh: !state.autoRefresh })),
    }),
    {
      name: 'vts-settings',
    }
  )
)
