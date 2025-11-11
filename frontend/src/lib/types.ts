// Type definitions matching backend API schemas

// ============================================================================
// Stream Management (Module 13)
// ============================================================================

export interface StreamConfig {
  name: string
  svID: string
  appIdHex: string
  macDst: string
  vlanId: number
  vlanPriority: number
  datSet: string
  confRev: number
  smpRate: number
  noASDU: number
  noChannels: number
}

export interface Stream extends StreamConfig {
  id: string
  status: 'stopped' | 'running'
  createdAt: string
  updatedAt: string
}

// ============================================================================
// Phasor Data (Module 2)
// ============================================================================

export interface PhasorChannel {
  mag: number
  angleDeg: number
}

export interface PhasorData {
  streamId: string
  freq: number
  channels: {
    'V-A': PhasorChannel
    'V-B': PhasorChannel
    'V-C': PhasorChannel
    'I-A': PhasorChannel
    'I-B': PhasorChannel
    'I-C': PhasorChannel
    'V-N': PhasorChannel
    'I-N': PhasorChannel
  }
}

// ============================================================================
// Harmonics (Module 8)
// ============================================================================

export interface HarmonicComponent {
  n: number  // harmonic order: 1, 2, 3, 5, 7, ...
  mag: number  // magnitude (percentage of fundamental)
  angleDeg: number  // angle in degrees
}

export interface HarmonicsData {
  streamId: string
  channel: string  // e.g., 'I-A', 'V-B'
  harmonics: HarmonicComponent[]
}

// ============================================================================
// COMTRADE Playback (Module 1)
// ============================================================================

export interface ComtradeMetadata {
  filename: string
  sampleRate: number
  totalSamples: number
  channels: {
    name: string
    type: 'analog' | 'digital'
    unit: string
  }[]
}

export interface ComtradeMapping {
  fileChannel: string
  svChannel: string
}

export interface ComtradePlayback {
  streamId: string
  file: File
  mapping: ComtradeMapping[]
  loop: boolean
}

// ============================================================================
// Sequencer (Module 3)
// ============================================================================

export interface SequenceState {
  name: string
  duration: number  // milliseconds
  phasors: Partial<PhasorData>
}

export interface Sequence {
  id: string
  name: string
  streamId: string
  states: SequenceState[]
  loop: boolean
}

export interface SequenceRun {
  sequenceId: string
  streamId: string
  loop: boolean
}

// ============================================================================
// GOOSE (Module 4)
// ============================================================================

export interface GooseMessage {
  appId: string
  goCBRef: string
  macSrc: string
  lastSeen: string
  dataSet?: string
}

export interface GooseSubscription {
  id: string
  appId: string
  goCBRef: string
  tripRule: string  // JavaScript expression
  active: boolean
}

export interface TripFlag {
  isTripped: boolean
  timestamp?: string
  source?: string
}

// ============================================================================
// Analyzer (Module 5)
// ============================================================================

export interface WaveformData {
  streamId: string
  channel: string
  samples: number[]
  timestamps: number[]
}

export interface PhasorTableRow {
  channel: string
  magnitude: number
  angle: number
  frequency: number
}

export interface VectorDiagramPoint {
  name: string
  x: number
  y: number
}

export interface HarmonicsSpectrum {
  channel: string
  harmonics: {
    order: number
    magnitude: number
    percentage: number
  }[]
}

// ============================================================================
// Impedance Injection (Module 6)
// ============================================================================

export interface ImpedanceConfig {
  streamId: string
  R: number  // ohms
  X: number  // ohms
  faultType: 'AG' | 'BG' | 'CG' | 'AB' | 'BC' | 'CA' | 'ABC'
  source: {
    RS1: number
    XS1: number
    RS0: number
    XS0: number
    Vprefault: number
  }
}

// ============================================================================
// Ramping Test (Module 7)
// ============================================================================

export interface RampConfig {
  streamId: string
  variable: string  // e.g., 'I-A.mag'
  startValue: number
  endValue: number
  stepSize: number
  stepDuration: number  // milliseconds
  stopOnTrip: boolean
  findDropoff: boolean
}

export interface RampingTestConfig extends RampConfig {}

export interface RampResult {
  pickupValue?: number
  pickupTime?: number
  dropoffValue?: number
  dropoffTime?: number
  resetRatio?: number
  tripped: boolean
}

export interface TestProgress {
  testType: string
  progress: number  // 0-100
  message: string
  currentStep?: number
  totalSteps?: number
}

export interface TestResult {
  testId: string
  testType: string
  success: boolean
  duration: number
  results: RampResult | DistanceTestResult[] | OvercurrentTestResult[] | DifferentialTestResult[]
}

// ============================================================================
// Distance 21 Test (Module 9)
// ============================================================================

export interface DistanceTestPoint {
  R: number
  X: number
  faultType: 'AG' | 'BG' | 'CG' | 'AB' | 'BC' | 'CA' | 'ABC'
}

export interface DistanceTestConfig {
  streamId: string
  source: ImpedanceConfig['source']
  points: DistanceTestPoint[]
}

export interface DistanceTestResult {
  point: DistanceTestPoint
  tripTime?: number
  tripped: boolean
  pass: boolean
}

// ============================================================================
// Overcurrent 50/51 Test (Module 10)
// ============================================================================

export interface OvercurrentTestConfig {
  streamId: string
  pickup: number
  tms: number
  curve: 'IEC_SI' | 'IEC_VI' | 'IEC_EI' | 'IEEE_MI' | 'IEEE_VI' | 'IEEE_EI'
  testPoints: number[]  // multiples of pickup, e.g., [1.1, 1.5, 2, 3, 5]
}

export interface OvercurrentTestResult {
  multiple: number
  current: number
  expectedTime: number
  actualTime?: number
  tripped: boolean
  pass: boolean
}

// ============================================================================
// Differential 87 Test (Module 11)
// ============================================================================

export interface DifferentialTestPoint {
  Ir: number  // restraint current
  Id: number  // differential current
}

export interface DifferentialTestConfig {
  side1: string  // streamId
  side2: string  // streamId
  points: DifferentialTestPoint[]
  settings: {
    slope1: number
    slope2: number
    breakpoint: number
    minPickup: number
  }
}

export interface DifferentialTestResult {
  point: DifferentialTestPoint
  tripTime?: number
  tripped: boolean
  pass: boolean
}

// ============================================================================
// WebSocket Events
// ============================================================================

export interface WsEvent {
  type: string
  data: unknown
}

export interface StreamStatusEvent extends WsEvent {
  type: 'stream/status'
  data: {
    streamId: string
    status: 'running' | 'stopped' | 'error'
  }
}

export interface TripEvent extends WsEvent {
  type: 'goose/trip'
  data: TripFlag
}

export interface AnalyzerDataEvent extends WsEvent {
  type: 'analyzer/data'
  data: {
    phasors?: PhasorTableRow[]
    waveform?: WaveformData
    harmonics?: HarmonicsSpectrum
  }
}

export interface TestProgressEvent extends WsEvent {
  type: 'test/progress'
  data: {
    testId: string
    progress: number
    message: string
  }
}

export interface TestResultEvent extends WsEvent {
  type: 'test/result'
  data: {
    testId: string
    results: RampResult | DistanceTestResult[] | OvercurrentTestResult[] | DifferentialTestResult[]
  }
}

// ============================================================================
// API Response Types
// ============================================================================

export interface ApiResponse<T = unknown> {
  success: boolean
  data?: T
  error?: string
  message?: string
}

export interface StreamsResponse {
  streams: Stream[]
}

export interface GooseMessagesResponse {
  messages: GooseMessage[]
}

export interface GooseSubscriptionsResponse {
  subscriptions: GooseSubscription[]
}

// ============================================================================
// Error Types
// ============================================================================

export class ApiError extends Error {
  statusCode?: number
  details?: unknown

  constructor(message: string, statusCode?: number, details?: unknown) {
    super(message)
    this.name = 'ApiError'
    this.statusCode = statusCode
    this.details = details
  }
}

export class WebSocketError extends Error {
  code?: number

  constructor(message: string, code?: number) {
    super(message)
    this.name = 'WebSocketError'
    this.code = code
  }
}
