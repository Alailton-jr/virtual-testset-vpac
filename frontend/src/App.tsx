import { BrowserRouter, Routes, Route, Navigate } from 'react-router-dom'
import { ThemeProvider } from '@/components/theme-provider'
import { AppLayout } from '@/layouts/AppLayout'
import { Dashboard } from '@/pages/Dashboard'
import StreamsPage from '@/pages/StreamsPage'
import ManualInjectionPage from '@/pages/ManualInjectionPage'
import ComtradePage from '@/pages/ComtradePage'
import SequencerPage from '@/pages/SequencerPage'
import AnalyzerPage from '@/pages/AnalyzerPage'
import GoosePage from '@/pages/GoosePage'
import ImpedancePage from '@/pages/ImpedancePage'
import RampingTestPage from '@/pages/RampingTestPage'
import DistanceTestPage from '@/pages/DistanceTestPage'
import OvercurrentTestPage from '@/pages/OvercurrentTestPage'
import DifferentialTestPage from '@/pages/DifferentialTestPage'
import SettingsPage from '@/pages/SettingsPage'
import BackendLogsPage from '@/pages/BackendLogsPage'
import './App.css'

function App() {
  return (
    <ThemeProvider defaultTheme="dark" storageKey="vts-ui-theme">
      <BrowserRouter>
        <AppLayout>
          <Routes>
            <Route path="/" element={<Dashboard />} />
            <Route path="/streams" element={<StreamsPage />} />
            <Route path="/manual" element={<ManualInjectionPage />} />
            <Route path="/comtrade" element={<ComtradePage />} />
            <Route path="/sequencer" element={<SequencerPage />} />
            <Route path="/analyzer" element={<AnalyzerPage />} />
            <Route path="/goose" element={<GoosePage />} />
            <Route path="/impedance" element={<ImpedancePage />} />
            <Route path="/ramping" element={<RampingTestPage />} />
            <Route path="/distance" element={<DistanceTestPage />} />
            <Route path="/overcurrent" element={<OvercurrentTestPage />} />
            <Route path="/differential" element={<DifferentialTestPage />} />
            <Route path="/settings" element={<SettingsPage />} />
            <Route path="/logs" element={<BackendLogsPage />} />
            <Route path="*" element={<Navigate to="/" replace />} />
          </Routes>
        </AppLayout>
      </BrowserRouter>
    </ThemeProvider>
  )
}

export default App


