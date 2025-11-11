import { create } from 'zustand'
import type { GooseMessage, GooseSubscription, TripFlag } from '@/lib/types'
import { api } from '@/lib/api'
import { ws } from '@/lib/ws'

interface GooseStore {
  // State
  subscriptions: GooseSubscription[]
  discoveredMessages: GooseMessage[]
  tripFlag: TripFlag | null
  isLoading: boolean
  error: string | null

  // Actions
  discoverGoose: () => Promise<void>
  fetchSubscriptions: () => Promise<void>
  createSubscription: (sub: Omit<GooseSubscription, 'id'>) => Promise<GooseSubscription>
  updateSubscription: (id: string, sub: Partial<GooseSubscription>) => Promise<void>
  deleteSubscription: (id: string) => Promise<void>
  resetTripFlag: () => Promise<void>
  clearError: () => void
  handleGooseMessage: (message: GooseMessage) => void
  handleTripFlag: (flag: TripFlag) => void
}

export const useGooseStore = create<GooseStore>((set) => ({
  // Initial state
  subscriptions: [],
  discoveredMessages: [],
  tripFlag: null,
  isLoading: false,
  error: null,

  // Discover GOOSE messages
  discoverGoose: async () => {
    set({ isLoading: true, error: null })
    try {
      const response = await api.discoverGoose()
      set({ discoveredMessages: response.messages, isLoading: false })
    } catch (error) {
      const message = error instanceof Error ? error.message : 'Failed to discover GOOSE'
      set({ error: message, isLoading: false })
      throw error
    }
  },

  // Fetch all subscriptions
  fetchSubscriptions: async () => {
    set({ isLoading: true, error: null })
    try {
      const subscriptions = await api.getGooseSubscriptions()
      set({ subscriptions, isLoading: false })
    } catch (error) {
      const message = error instanceof Error ? error.message : 'Failed to fetch subscriptions'
      set({ error: message, isLoading: false })
      throw error
    }
  },

  // Create new subscription
  createSubscription: async (sub) => {
    set({ isLoading: true, error: null })
    try {
      const newSub = await api.createGooseSubscription(sub)
      set((state) => ({
        subscriptions: [...state.subscriptions, newSub],
        isLoading: false,
      }))
      return newSub
    } catch (error) {
      const message = error instanceof Error ? error.message : 'Failed to create subscription'
      set({ error: message, isLoading: false })
      throw error
    }
  },

  // Update subscription
  updateSubscription: async (id, sub) => {
    set({ isLoading: true, error: null })
    try {
      const updatedSub = await api.updateGooseSubscription(id, sub)
      set((state) => ({
        subscriptions: state.subscriptions.map((s) =>
          s.id === id ? updatedSub : s
        ),
        isLoading: false,
      }))
    } catch (error) {
      const message = error instanceof Error ? error.message : 'Failed to update subscription'
      set({ error: message, isLoading: false })
      throw error
    }
  },

  // Delete subscription
  deleteSubscription: async (id) => {
    set({ isLoading: true, error: null })
    try {
      await api.deleteGooseSubscription(id)
      set((state) => ({
        subscriptions: state.subscriptions.filter((s) => s.id !== id),
        isLoading: false,
      }))
    } catch (error) {
      const message = error instanceof Error ? error.message : 'Failed to delete subscription'
      set({ error: message, isLoading: false })
      throw error
    }
  },

  // Reset trip flag
  resetTripFlag: async () => {
    set({ error: null })
    try {
      await api.resetTripFlag()
      set({ tripFlag: null })
    } catch (error) {
      const message = error instanceof Error ? error.message : 'Failed to reset trip flag'
      set({ error: message })
      throw error
    }
  },

  // Clear error
  clearError: () => set({ error: null }),

  // Handle WebSocket GOOSE message
  handleGooseMessage: (message) =>
    set((state) => ({
      discoveredMessages: [message, ...state.discoveredMessages].slice(0, 100), // Keep last 100
    })),

  // Handle trip flag update
  handleTripFlag: (flag) => set({ tripFlag: flag }),
}))

// Subscribe to GOOSE events
ws.subscribe('goose/message', (event) => {
  useGooseStore.getState().handleGooseMessage(event.data as GooseMessage)
})

ws.subscribe('goose/trip', (event) => {
  useGooseStore.getState().handleTripFlag(event.data as TripFlag)
})
