import type {
  Stream,
  StreamConfig,
  StreamsResponse,
  PhasorData,
  HarmonicsData,
  ComtradePlayback,
  ComtradeMetadata,
  SequenceRun,
  Sequence,
  GooseMessagesResponse,
  GooseSubscription,
  GooseSubscriptionsResponse,
  ImpedanceConfig,
  RampConfig,
  RampResult,
  DistanceTestConfig,
  DistanceTestResult,
  OvercurrentTestConfig,
  OvercurrentTestResult,
  DifferentialTestConfig,
  DifferentialTestResult,
  ApiResponse,
} from './types'

import { ApiError } from './types'

/**
 * REST API Client for Virtual TestSet backend
 * 
 * Provides methods for all backend REST endpoints:
 * - Stream management (Module 13)
 * - Phasor injection (Module 2)
 * - COMTRADE playback (Module 1)
 * - Sequencer (Module 3)
 * - GOOSE configuration (Module 4)
 * - Analyzer (Module 5)
 * - Impedance injection (Module 6)
 * - Ramping tests (Module 7)
 * - Distance tests (Module 9)
 * - Overcurrent tests (Module 10)
 * - Differential tests (Module 11)
 */
class ApiClient {
  private baseUrl: string

  constructor(baseUrl: string = '/api/v1') {
    this.baseUrl = baseUrl
  }

  /**
   * Generic fetch wrapper with error handling
   */
  private async request<T>(
    endpoint: string,
    options: RequestInit = {}
  ): Promise<T> {
    const url = `${this.baseUrl}${endpoint}`
    
    try {
      const response = await fetch(url, {
        ...options,
        headers: {
          'Content-Type': 'application/json',
          ...options.headers,
        },
      })

      if (!response.ok) {
        const error = await response.json().catch(() => ({}))
        throw new ApiError(
          error.message || `HTTP ${response.status}: ${response.statusText}`,
          response.status,
          error
        )
      }

      return await response.json()
    } catch (error) {
      if (error instanceof ApiError) {
        throw error
      }
      throw new ApiError(
        error instanceof Error ? error.message : 'Network error',
        0,
        error
      )
    }
  }

  // ============================================================================
  // Stream Management (Module 13)
  // ============================================================================

  async getStreams(): Promise<Stream[]> {
    const response = await this.request<StreamsResponse | Stream[]>('/streams')
    // Backend currently returns array directly, but StreamsResponse expects {streams: [...]}
    return Array.isArray(response) ? response : response.streams
  }

  async getStream(id: string): Promise<Stream> {
    return this.request<Stream>(`/streams/${id}`)
  }

  async createStream(config: StreamConfig): Promise<Stream> {
    return this.request<Stream>('/streams', {
      method: 'POST',
      body: JSON.stringify(config),
    })
  }

  async updateStream(id: string, config: Partial<StreamConfig>): Promise<Stream> {
    return this.request<Stream>(`/streams/${id}`, {
      method: 'PATCH',
      body: JSON.stringify(config),
    })
  }

  async deleteStream(id: string): Promise<void> {
    await this.request<void>(`/streams/${id}`, {
      method: 'DELETE',
    })
  }

  async startStream(id: string): Promise<ApiResponse> {
    return this.request<ApiResponse>(`/streams/${id}/start`, {
      method: 'POST',
    })
  }

  async stopStream(id: string): Promise<ApiResponse> {
    return this.request<ApiResponse>(`/streams/${id}/stop`, {
      method: 'POST',
    })
  }

  // ============================================================================
  // Phasor Injection (Module 2)
  // ============================================================================

  async updatePhasors(streamId: string, data: Omit<PhasorData, 'streamId'>): Promise<ApiResponse> {
    return this.request<ApiResponse>(`/streams/${streamId}/phasors`, {
      method: 'POST',
      body: JSON.stringify(data),
    })
  }

  // ============================================================================
  // Harmonics (Module 8)
  // ============================================================================

  async updateHarmonics(streamId: string, data: Omit<HarmonicsData, 'streamId'>): Promise<ApiResponse> {
    return this.request<ApiResponse>(`/streams/${streamId}/harmonics`, {
      method: 'POST',
      body: JSON.stringify(data),
    })
  }

  // ============================================================================
  // COMTRADE Playback (Module 1)
  // ============================================================================

  async uploadComtrade(file: File): Promise<ComtradeMetadata> {
    const formData = new FormData()
    formData.append('file', file)

    const url = `${this.baseUrl}/comtrade/upload`
    const response = await fetch(url, {
      method: 'POST',
      body: formData,
    })

    if (!response.ok) {
      throw new ApiError(`Upload failed: ${response.statusText}`, response.status)
    }

    return response.json()
  }

  async startComtradePlayback(config: ComtradePlayback): Promise<ApiResponse> {
    const formData = new FormData()
    formData.append('file', config.file)
    formData.append('streamId', config.streamId)
    formData.append('mapping', JSON.stringify(config.mapping))
    formData.append('loop', String(config.loop))

    const url = `${this.baseUrl}/comtrade/play`
    const response = await fetch(url, {
      method: 'POST',
      body: formData,
    })

    if (!response.ok) {
      throw new ApiError(`Playback failed: ${response.statusText}`, response.status)
    }

    return response.json()
  }

  async stopComtradePlayback(streamId: string): Promise<ApiResponse> {
    return this.request<ApiResponse>(`/comtrade/${streamId}/stop`, {
      method: 'POST',
    })
  }

  // ============================================================================
  // Sequencer (Module 3)
  // ============================================================================

  async getSequences(): Promise<Sequence[]> {
    const response = await this.request<{ sequences: Sequence[] }>('/sequences')
    return response.sequences
  }

  async createSequence(sequence: Omit<Sequence, 'id'>): Promise<Sequence> {
    return this.request<Sequence>('/sequences', {
      method: 'POST',
      body: JSON.stringify(sequence),
    })
  }

  async updateSequence(id: string, sequence: Partial<Sequence>): Promise<Sequence> {
    return this.request<Sequence>(`/sequences/${id}`, {
      method: 'PATCH',
      body: JSON.stringify(sequence),
    })
  }

  async deleteSequence(id: string): Promise<void> {
    await this.request<void>(`/sequences/${id}`, {
      method: 'DELETE',
    })
  }

  async runSequence(config: SequenceRun): Promise<ApiResponse> {
    return this.request<ApiResponse>('/sequences/run', {
      method: 'POST',
      body: JSON.stringify(config),
    })
  }

  async stopSequence(streamId: string): Promise<ApiResponse> {
    return this.request<ApiResponse>(`/sequences/${streamId}/stop`, {
      method: 'POST',
    })
  }

  // ============================================================================
  // GOOSE (Module 4)
  // ============================================================================

  async discoverGoose(): Promise<GooseMessagesResponse> {
    return this.request<GooseMessagesResponse>('/goose/discover')
  }

  async getGooseSubscriptions(): Promise<GooseSubscription[]> {
    const response = await this.request<GooseSubscriptionsResponse>('/goose/subscriptions')
    return response.subscriptions
  }

  async createGooseSubscription(subscription: Omit<GooseSubscription, 'id'>): Promise<GooseSubscription> {
    return this.request<GooseSubscription>('/goose/subscriptions', {
      method: 'POST',
      body: JSON.stringify(subscription),
    })
  }

  async updateGooseSubscription(id: string, subscription: Partial<GooseSubscription>): Promise<GooseSubscription> {
    return this.request<GooseSubscription>(`/goose/subscriptions/${id}`, {
      method: 'PATCH',
      body: JSON.stringify(subscription),
    })
  }

  async deleteGooseSubscription(id: string): Promise<void> {
    await this.request<void>(`/goose/subscriptions/${id}`, {
      method: 'DELETE',
    })
  }

  async resetTripFlag(): Promise<ApiResponse> {
    return this.request<ApiResponse>('/goose/trip/reset', {
      method: 'POST',
    })
  }

  // ============================================================================
  // Analyzer (Module 5)
  // ============================================================================

  async startAnalyzer(streamId: string): Promise<ApiResponse> {
    return this.request<ApiResponse>('/analyzer/start', {
      method: 'POST',
      body: JSON.stringify({ streamId }),
    })
  }

  async stopAnalyzer(): Promise<ApiResponse> {
    return this.request<ApiResponse>('/analyzer/stop', {
      method: 'POST',
    })
  }

  // ============================================================================
  // Impedance Injection (Module 6)
  // ============================================================================

  async applyImpedance(config: ImpedanceConfig): Promise<ApiResponse> {
    return this.request<ApiResponse>('/impedance/apply', {
      method: 'POST',
      body: JSON.stringify(config),
    })
  }

  async clearImpedance(streamId: string): Promise<ApiResponse> {
    return this.request<ApiResponse>('/impedance/clear', {
      method: 'POST',
      body: JSON.stringify({ streamId }),
    })
  }

  // ============================================================================
  // Ramping Test (Module 7)
  // ============================================================================

  async runRampingTest(config: RampConfig): Promise<RampResult> {
    return this.request<RampResult>('/ramping/run', {
      method: 'POST',
      body: JSON.stringify(config),
    })
  }

  async stopRampingTest(streamId: string): Promise<ApiResponse> {
    return this.request<ApiResponse>(`/ramping/${streamId}/stop`, {
      method: 'POST',
    })
  }

  // ============================================================================
  // Distance Test (Module 9)
  // ============================================================================

  async runDistanceTest(config: DistanceTestConfig): Promise<DistanceTestResult[]> {
    const response = await this.request<{ results: DistanceTestResult[] }>('/distance/run', {
      method: 'POST',
      body: JSON.stringify(config),
    })
    return response.results
  }

  async stopDistanceTest(streamId: string): Promise<ApiResponse> {
    return this.request<ApiResponse>(`/distance/${streamId}/stop`, {
      method: 'POST',
    })
  }

  // ============================================================================
  // Overcurrent Test (Module 10)
  // ============================================================================

  async runOvercurrentTest(config: OvercurrentTestConfig): Promise<OvercurrentTestResult[]> {
    const response = await this.request<{ results: OvercurrentTestResult[] }>('/overcurrent/run', {
      method: 'POST',
      body: JSON.stringify(config),
    })
    return response.results
  }

  async stopOvercurrentTest(streamId: string): Promise<ApiResponse> {
    return this.request<ApiResponse>(`/overcurrent/${streamId}/stop`, {
      method: 'POST',
    })
  }

  // ============================================================================
  // Differential Test (Module 11)
  // ============================================================================

  async runDifferentialTest(config: DifferentialTestConfig): Promise<DifferentialTestResult[]> {
    const response = await this.request<{ results: DifferentialTestResult[] }>('/differential/run', {
      method: 'POST',
      body: JSON.stringify(config),
    })
    return response.results
  }

  async stopDifferentialTest(side1: string, side2: string): Promise<ApiResponse> {
    return this.request<ApiResponse>('/differential/stop', {
      method: 'POST',
      body: JSON.stringify({ side1, side2 }),
    })
  }

  // ============================================================================
  // Health Check
  // ============================================================================

  async healthCheck(): Promise<{ status: string; timestamp: number; version: string }> {
    return this.request<{ status: string; timestamp: number; version: string }>('/health')
  }

  // ============================================================================
  // System Status
  // ============================================================================

  async getSequenceStatus(): Promise<{ running: boolean; currentStep?: number; totalSteps?: number }> {
    const response = await this.request<{ status: string; currentState: number; totalElapsed: number; stateElapsed: number }>('/sequences/status')
    // Backend returns { status: "idle" | "running", ... }
    // Transform to match frontend expectation
    return {
      running: response.status === 'running',
      currentStep: response.currentState >= 0 ? response.currentState : undefined,
      totalSteps: undefined, // Backend doesn't provide this
    }
  }

  async getAnalyzerStatus(): Promise<{ active: boolean; streamId?: string }> {
    const response = await this.request<{ running: boolean; streamMac: string }>('/analyzer/status')
    // Backend returns { running: boolean, streamMac: string }
    // Transform to match frontend expectation
    return {
      active: response.running,
      streamId: response.streamMac || undefined,
    }
  }

  // ============================================================================
  // Backend Logs
  // ============================================================================

  async getBackendLogs(limit: number = 100): Promise<{ logs: Array<{ timestamp: string; level: string; message: string }> }> {
    return this.request(`/logs?limit=${limit}`)
  }

  getLogsWebSocket(): WebSocket {
    const wsUrl = this.baseUrl.replace(/^http/, 'ws').replace(/\/api\/v1$/, '/ws/logs')
    return new WebSocket(wsUrl)
  }

  async getNetworkInterfaces(): Promise<{
    interfaces: Array<{
      name: string
      active: boolean
      macAddress?: string
      ipAddress?: string
    }>
    platform: string
  }> {
    return this.request('/system/network-interfaces')
  }
}

// Export singleton instance
export const api = new ApiClient()

// Export class for testing
export { ApiClient }
