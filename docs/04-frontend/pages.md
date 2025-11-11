# Frontend Pages Guide

> This guide describes each page in the frontend, its route, purpose, primary components/state, and a screenshot placeholder. Images should be placed under `docs/images/`.

## General Pages

### Page: Dashboard
- **Route:** `/`
- **Purpose:** High-level system status, recent activity, and quick access to key features
- **Key Components:** `StatusCards`, `RecentActivity`, `QuickActions`, `SystemMetrics`
- **Primary State/Queries:** `useSystemStatus()`, `useStreamStore()`, `useAlerts()`
- **Image:** ![Dashboard](../images/dashboard.png) <!-- TODO: add final screenshot -->

### Page: Settings
- **Route:** `/settings`
- **Purpose:** Application configuration including API endpoints, theme preferences, and system parameters
- **Key Components:** `SettingsForm`, `ThemeToggle`, `APIConfiguration`
- **Primary State/Queries:** `useSettings()`, `useUpdateSettings()`
- **Image:** ![Settings](../images/settings.png) <!-- TODO: add final screenshot -->

### Page: Backend Logs
- **Route:** `/logs`
- **Purpose:** Real-time backend server logs and terminal output for debugging
- **Key Components:** `LogViewer`, `LogFilters`, `ConnectionStatus`
- **Primary State/Queries:** WebSocket connection to `/ws/logs`, `useLogs()`
- **Image:** ![Backend Logs](../images/backend-logs.png) <!-- TODO: add final screenshot -->

## IEC 61850 Configuration

### Page: SV Publishers (Streams)
- **Route:** `/streams`
- **Purpose:** Manage and monitor IEC 61850-9-2 Sampled Value streams
- **Key Components:** `StreamTable`, `StreamConfigDialog`, `StreamMetrics`, `PhasorDiagram`
- **Primary State/Queries:** `useStreamStore()`, `api.getStreams()`, `api.updatePhasors()`
- **Image:** ![Streams](../images/streams.png) <!-- TODO: add final screenshot -->

### Page: GOOSE Configuration
- **Route:** `/goose`
- **Purpose:** Configure GOOSE (Generic Object Oriented Substation Event) messaging
- **Key Components:** `GOOSEPublisherConfig`, `GOOSESubscriberList`, `DataSetEditor`
- **Primary State/Queries:** `useGOOSEStore()`, `api.getGOOSEConfig()`, `api.updateGOOSE()`
- **Image:** ![GOOSE Config](../images/goose.png) <!-- TODO: add final screenshot -->

### Page: Analyzer
- **Route:** `/analyzer`
- **Purpose:** Real-time analysis of IEC 61850 traffic and protocol debugging
- **Key Components:** `PacketCapture`, `ProtocolDecoder`, `TrafficMetrics`, `FilterPanel`
- **Primary State/Queries:** `useAnalyzerStore()`, WebSocket to `/ws/analyzer`
- **Image:** ![Analyzer](../images/analyzer.png) <!-- TODO: add final screenshot -->

## Basic Test Features

### Page: Manual Injection
- **Route:** `/manual`
- **Purpose:** Manually inject voltage and current phasors for ad-hoc testing
- **Key Components:** `PhasorInputs`, `PhaseControls`, `RealTimeUpdates`, `PhasorDiagram`
- **Primary State/Queries:** `useManualInjection()`, `api.updatePhasors()`, `api.updateHarmonics()`
- **Image:** ![Manual Injection](../images/manual-injection.png) <!-- TODO: add final screenshot -->

### Page: COMTRADE/CSV Playback
- **Route:** `/comtrade`
- **Purpose:** Upload and replay COMTRADE or CSV waveform files
- **Key Components:** `FileUpload`, `WaveformViewer`, `PlaybackControls`, `TimelineSeeker`
- **Primary State/Queries:** `useComtradeStore()`, `api.uploadCOMTRADE()`, `api.startPlayback()`
- **Image:** ![COMTRADE Playback](../images/comtrade.png) <!-- TODO: add final screenshot -->

### Page: Sequencer
- **Route:** `/sequencer`
- **Purpose:** Create and execute multi-step test sequences with timing control
- **Key Components:** `SequenceEditor`, `StepList`, `ExecutionControls`, `ProgressIndicator`
- **Primary State/Queries:** `useSequencer()`, `api.runSequence()`, `api.getSequenceStatus()`
- **Image:** ![Sequencer](../images/sequencer.png) <!-- TODO: add final screenshot -->

### Page: Impedance Injection
- **Route:** `/impedance`
- **Purpose:** Inject impedance values for relay testing
- **Key Components:** `ImpedanceConfig`, `RXDiagram`, `ImpedanceTable`, `TestControls`
- **Primary State/Queries:** `useImpedance()`, `api.injectImpedance()`, `api.getResults()`
- **Image:** ![Impedance](../images/impedance.png) <!-- TODO: add final screenshot -->

## Specific Protection Tests

### Page: Ramping Test
- **Route:** `/ramping`
- **Purpose:** Perform ramping tests to determine pickup/dropout values
- **Key Components:** `RampConfig`, `VariableSelector`, `RampChart`, `KPIDisplay`
- **Primary State/Queries:** `useRamping()`, `api.startRamp()`, `api.getRampResults()`
- **Features:**
  - Variable selection (voltage, current, frequency)
  - Configurable start, end, step values
  - Pickup, dropout, and reset time measurement
- **Image:** ![Ramping Test](../images/ramping.png) <!-- TODO: add final screenshot -->

### Page: Distance Protection (21)
- **Route:** `/distance`
- **Purpose:** Test distance protection elements with R-X impedance plane
- **Key Components:** `ImpedancePointEditor`, `RXDiagram`, `ZoneConfig`, `TestPointTable`
- **Primary State/Queries:** `useDistance()`, `api.testDistance()`, `api.getZoneResults()`
- **Features:**
  - R-X impedance plane visualization
  - Multiple zone configuration
  - Fault angle sweep
  - Trip time measurement
- **Image:** ![Distance Test](../images/distance.png) <!-- TODO: add final screenshot -->

### Page: Overcurrent Protection (50/51)
- **Route:** `/overcurrent`
- **Purpose:** Test overcurrent protection with definite and inverse time curves
- **Key Components:** `CurveSelector`, `PickupConfig`, `TimeDial`, `TestResultsTable`
- **Primary State/Queries:** `useOvercurrent()`, `api.testOvercurrent()`, `api.getTimingResults()`
- **Features:**
  - ANSI/IEC curve selection
  - Pickup current configuration
  - Time dial/TMS adjustment
  - Multiple test points
- **Image:** ![Overcurrent Test](../images/overcurrent.png) <!-- TODO: add final screenshot -->

### Page: Differential Protection (87)
- **Route:** `/differential`
- **Purpose:** Test differential protection with operating and restraint characteristics
- **Key Components:** `SlopeConfig`, `CharacteristicDiagram`, `StreamSelector`, `TestPointEditor`
- **Primary State/Queries:** `useDifferential()`, `api.testDifferential()`, `api.getDiffResults()`
- **Features:**
  - Dual slope configuration
  - Operating current vs. restraint current plot
  - Two-stream coordination
  - Harmonic restraint testing
- **Image:** ![Differential Test](../images/differential.png) <!-- TODO: add final screenshot -->

---

## Common UI Patterns

### Stream Selection
Most test pages include a stream selector dropdown to choose which SV publisher to use for the test.

### Test Execution Flow
1. Configure test parameters
2. Select target stream(s)
3. Click "Run Test" or "Start"
4. View real-time progress
5. Review results table/chart

### Results Display
- Pass/fail indicators (badges)
- Timing measurements (ms precision)
- Tabular results with export options
- Real-time charts and visualizations

### State Management
- Zustand stores for global state
- React Query for API calls
- WebSocket connections for real-time data

---

## Adding New Pages

To add a new page to the frontend:

1. Create component in `frontend/src/pages/`
2. Add route to `frontend/src/App.tsx`
3. Add navigation item to `frontend/src/components/Sidebar.tsx`
4. Create API methods in `frontend/src/lib/api.ts`
5. Add type definitions to `frontend/src/lib/types.ts`
6. Update this documentation with page details
7. Take screenshot and add to `docs/images/`
