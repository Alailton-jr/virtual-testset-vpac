# Frontend Architecture

## Overview

The frontend is a modern React application built with TypeScript, providing a responsive UI for configuring, executing, and monitoring protection relay tests.

## Technology Stack

- **Framework:** React 18
- **Language:** TypeScript 5
- **Build Tool:** Vite
- **UI Library:** shadcn/ui (Radix UI + Tailwind CSS)
- **State Management:** Zustand
- **HTTP Client:** Axios
- **Routing:** React Router v6
- **Icons:** Lucide React
- **Charts:** Recharts
- **Testing:** Vitest + React Testing Library

## Directory Structure

```
frontend/
├── src/
│   ├── pages/              # Page components (one per route)
│   ├── components/         # Reusable UI components
│   │   ├── ui/            # shadcn/ui primitives
│   │   └── ...            # Custom components
│   ├── stores/            # Zustand state stores
│   ├── lib/               # Utilities and API client
│   │   ├── api.ts         # HTTP/WebSocket API
│   │   ├── types.ts       # TypeScript types
│   │   └── utils.ts       # Helper functions
│   ├── App.tsx            # Main app component + routing
│   ├── main.tsx           # Entry point
│   └── index.css          # Global styles + Tailwind
├── public/                # Static assets
├── tests/                 # Component tests
├── package.json           # Dependencies
├── vite.config.ts         # Vite configuration
├── tailwind.config.ts     # Tailwind configuration
└── tsconfig.json          # TypeScript configuration
```

## Key Components

### 1. Routing (`App.tsx`)

React Router v6 with routes:

```typescript
<Routes>
  <Route path="/" element={<Dashboard />} />
  <Route path="/streams" element={<StreamsPage />} />
  <Route path="/manual" element={<ManualInjectionPage />} />
  <Route path="/overcurrent" element={<OvercurrentTestPage />} />
  // ... more routes
</Routes>
```

### 2. State Management

**Zustand Stores:**

- `useStreamStore` - SV stream configuration
- `useGOOSEStore` - GOOSE configuration
- `useSettingsStore` - Application settings
- `useAnalyzerStore` - Packet analyzer state

**Example Store:**

```typescript
interface StreamStore {
  streams: Stream[]
  fetchStreams: () => Promise<void>
  updateStream: (id: string, data: Partial<Stream>) => Promise<void>
}

export const useStreamStore = create<StreamStore>((set) => ({
  streams: [],
  fetchStreams: async () => {
    const streams = await api.getStreams()
    set({ streams })
  },
  // ...
}))
```

### 3. API Client (`lib/api.ts`)

Centralized API interface:

```typescript
export const api = {
  // Streams
  getStreams: () => axios.get<Stream[]>('/api/v1/streams'),
  updatePhasors: (id: string, phasors: Phasor[]) => 
    axios.post(`/api/v1/streams/${id}/phasors`, phasors),
  
  // Tests
  runOvercurrentTest: (config: OvercurrentConfig) => 
    axios.post('/api/v1/tests/overcurrent', config),
  
  // WebSocket
  getLogsWebSocket: () => new WebSocket('ws://localhost:8080/ws/logs'),
}
```

### 4. UI Components

**shadcn/ui primitives:**
- `Button`, `Input`, `Select`, `Card`, `Table`, etc.
- Accessible, themeable, customizable

**Custom components:**
- `PhasorDiagram` - Real-time phasor visualization
- `StreamTable` - Stream management table
- `TestResults` - Test result display
- `Sidebar` - Navigation sidebar

### 5. Pages

Each page is a self-contained component:

- **Dashboard:** System overview
- **Streams:** SV publisher configuration
- **Manual Injection:** Real-time phasor control
- **Test Pages:** Overcurrent, Distance, Differential, Ramping
- **COMTRADE:** File upload and playback
- **Analyzer:** Packet capture viewer
- **Settings:** Application configuration

See [Frontend Pages Guide](../04-frontend/pages.md) for details.

## Data Flow

### Page Load
1. Component mounts
2. Zustand store initialized
3. API call via `useEffect`
4. State updated
5. Component re-renders

### User Interaction
1. User clicks button / enters input
2. Handler function called
3. API request sent to backend
4. Response updates Zustand store
5. UI reflects new state

### Real-time Updates
1. WebSocket connection established
2. Backend streams data
3. Message handler updates local state
4. React re-renders affected components

## Styling

### Tailwind CSS
- Utility-first CSS framework
- Dark mode support via `dark:` prefix
- Responsive design with `md:`, `lg:` breakpoints

### Theme System
CSS variables for colors:

```css
:root {
  --background: 0 0% 100%;
  --foreground: 222.2 84% 4.9%;
  --primary: 221.2 83.2% 53.3%;
  // ...
}

.dark {
  --background: 222.2 84% 4.9%;
  --foreground: 210 40% 98%;
  // ...
}
```

### Component Styling
```tsx
<Card className="p-4 bg-card border-border">
  <CardHeader>
    <CardTitle>Stream Configuration</CardTitle>
  </CardHeader>
</Card>
```

## TypeScript Types

Shared types in `lib/types.ts`:

```typescript
export interface Stream {
  id: string
  name: string
  svid: string
  dstMAC: string
  vlanId: number
  sampleRate: number
  phasors: Phasor[]
  status: 'stopped' | 'running' | 'error'
}

export interface Phasor {
  name: string
  magnitude: number
  angle: number
}

export interface TestResult {
  passed: boolean
  timestamp: string
  duration: number
  message: string
}
```

## Performance Optimization

- **Code Splitting:** Vite automatically splits routes
- **Lazy Loading:** `React.lazy()` for heavy components
- **Memoization:** `useMemo`, `useCallback` for expensive computations
- **WebSocket Throttling:** Limit update frequency for real-time data

## Build & Deploy

### Development

```bash
npm install
npm run dev        # Dev server on http://localhost:5173
```

### Production

```bash
npm run build      # Outputs to dist/
npm run preview    # Preview production build
```

### Docker

```dockerfile
FROM node:20-alpine AS build
WORKDIR /app
COPY package*.json ./
RUN npm ci
COPY . .
RUN npm run build

FROM nginx:alpine
COPY --from=build /app/dist /usr/share/nginx/html
COPY nginx.conf /etc/nginx/nginx.conf
```

## Testing

See [Unit Tests Guide](../06-tests/unit-tests.md).

```bash
npm test                 # Run all tests
npm run test:coverage    # With coverage report
```

## Environment Variables

None required. API URL auto-detected:

- Development: `http://localhost:8080`
- Production: Same origin as frontend

## Browser Support

- Chrome/Edge: Latest 2 versions
- Firefox: Latest 2 versions
- Safari: Latest 2 versions

## Accessibility

- ARIA labels on interactive elements
- Keyboard navigation support
- Screen reader compatible
- Focus management

## References

- [System Overview](./system-overview.md)
- [Frontend Pages Guide](../04-frontend/pages.md)
- [Backend Integration](../04-frontend/backend-integration.md)
