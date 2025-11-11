import type { WsEvent } from './types'
import { WebSocketError } from './types'

type EventCallback = (event: WsEvent) => void
type ConnectionStateCallback = (connected: boolean) => void

/**
 * WebSocket Client for Virtual TestSet backend
 * 
 * Features:
 * - Auto-reconnection with exponential backoff
 * - Event subscription system
 * - Connection state management
 * - Error handling
 * - Graceful disconnection
 * 
 * Usage:
 * ```typescript
 * const ws = new WsClient('ws://localhost:8080/ws')
 * 
 * // Connect
 * ws.connect()
 * 
 * // Subscribe to events
 * ws.subscribe('stream/status', (event) => {
 *   console.log('Stream status:', event.data)
 * })
 * 
 * // Disconnect
 * ws.disconnect()
 * ```
 */
export class WsClient {
  private ws: WebSocket | null = null
  private url: string
  private reconnectAttempts = 0
  private maxReconnectAttempts = 5
  private reconnectDelay = 1000 // Start with 1 second
  private maxReconnectDelay = 30000 // Max 30 seconds
  private reconnectTimer: number | null = null
  private subscriptions = new Map<string, Set<EventCallback>>()
  private connectionStateListeners = new Set<ConnectionStateCallback>()
  private isConnected = false
  private shouldReconnect = true

  constructor(url: string = 'ws://localhost:8080/ws') {
    this.url = url
  }

  /**
   * Connect to WebSocket server
   */
  connect(): void {
    if (this.ws?.readyState === WebSocket.OPEN) {
      console.warn('[WS] Already connected')
      return
    }

    if (this.ws?.readyState === WebSocket.CONNECTING) {
      console.warn('[WS] Connection already in progress')
      return
    }

    try {
      console.log('[WS] Connecting to', this.url)
      this.ws = new WebSocket(this.url)

      this.ws.onopen = this.handleOpen.bind(this)
      this.ws.onmessage = this.handleMessage.bind(this)
      this.ws.onerror = this.handleError.bind(this)
      this.ws.onclose = this.handleClose.bind(this)
    } catch (error) {
      console.error('[WS] Connection error:', error)
      this.scheduleReconnect()
    }
  }

  /**
   * Disconnect from WebSocket server
   */
  disconnect(): void {
    console.log('[WS] Disconnecting')
    this.shouldReconnect = false
    this.clearReconnectTimer()
    
    if (this.ws) {
      this.ws.close(1000, 'Client disconnect')
      this.ws = null
    }
    
    this.setConnectionState(false)
  }

  /**
   * Subscribe to specific event type
   */
  subscribe(eventType: string, callback: EventCallback): () => void {
    if (!this.subscriptions.has(eventType)) {
      this.subscriptions.set(eventType, new Set())
    }
    
    this.subscriptions.get(eventType)!.add(callback)
    
    // Return unsubscribe function
    return () => {
      const callbacks = this.subscriptions.get(eventType)
      if (callbacks) {
        callbacks.delete(callback)
        if (callbacks.size === 0) {
          this.subscriptions.delete(eventType)
        }
      }
    }
  }

  /**
   * Subscribe to all events (wildcard)
   */
  subscribeAll(callback: EventCallback): () => void {
    return this.subscribe('*', callback)
  }

  /**
   * Listen to connection state changes
   */
  onConnectionChange(callback: ConnectionStateCallback): () => void {
    this.connectionStateListeners.add(callback)
    
    // Immediately call with current state
    callback(this.isConnected)
    
    // Return unsubscribe function
    return () => {
      this.connectionStateListeners.delete(callback)
    }
  }

  /**
   * Send message to server
   */
  send(data: unknown): void {
    if (!this.ws || this.ws.readyState !== WebSocket.OPEN) {
      throw new WebSocketError('WebSocket is not connected')
    }

    try {
      this.ws.send(JSON.stringify(data))
    } catch (error) {
      console.error('[WS] Send error:', error)
      throw new WebSocketError(
        error instanceof Error ? error.message : 'Failed to send message'
      )
    }
  }

  /**
   * Get current connection state
   */
  get connected(): boolean {
    return this.isConnected
  }

  /**
   * Handle WebSocket open event
   */
  private handleOpen(): void {
    console.log('[WS] Connected')
    this.reconnectAttempts = 0
    this.reconnectDelay = 1000
    this.setConnectionState(true)
  }

  /**
   * Handle incoming WebSocket messages
   */
  private handleMessage(event: MessageEvent): void {
    try {
      const data = JSON.parse(event.data) as WsEvent
      
      // Notify wildcard subscribers
      const wildcardCallbacks = this.subscriptions.get('*')
      if (wildcardCallbacks) {
        wildcardCallbacks.forEach(callback => callback(data))
      }

      // Notify type-specific subscribers
      const callbacks = this.subscriptions.get(data.type)
      if (callbacks) {
        callbacks.forEach(callback => callback(data))
      }
    } catch (error) {
      console.error('[WS] Failed to parse message:', error)
    }
  }

  /**
   * Handle WebSocket error
   */
  private handleError(event: Event): void {
    console.error('[WS] Error:', event)
  }

  /**
   * Handle WebSocket close event
   */
  private handleClose(event: CloseEvent): void {
    console.log('[WS] Disconnected:', event.code, event.reason)
    this.setConnectionState(false)

    if (this.shouldReconnect) {
      this.scheduleReconnect()
    }
  }

  /**
   * Schedule reconnection attempt
   */
  private scheduleReconnect(): void {
    if (this.reconnectAttempts >= this.maxReconnectAttempts) {
      console.error('[WS] Max reconnect attempts reached')
      return
    }

    this.clearReconnectTimer()

    const delay = Math.min(
      this.reconnectDelay * Math.pow(2, this.reconnectAttempts),
      this.maxReconnectDelay
    )

    console.log(`[WS] Reconnecting in ${delay}ms (attempt ${this.reconnectAttempts + 1}/${this.maxReconnectAttempts})`)

    this.reconnectTimer = window.setTimeout(() => {
      this.reconnectAttempts++
      this.connect()
    }, delay)
  }

  /**
   * Clear reconnection timer
   */
  private clearReconnectTimer(): void {
    if (this.reconnectTimer !== null) {
      clearTimeout(this.reconnectTimer)
      this.reconnectTimer = null
    }
  }

  /**
   * Update connection state and notify listeners
   */
  private setConnectionState(connected: boolean): void {
    if (this.isConnected !== connected) {
      this.isConnected = connected
      this.connectionStateListeners.forEach(callback => callback(connected))
    }
  }
}

// Export singleton instance
export const ws = new WsClient()

// Re-export for convenience
export { WebSocketError }
