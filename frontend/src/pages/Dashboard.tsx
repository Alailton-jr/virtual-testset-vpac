import { useEffect, useState } from 'react'
import { Card, CardContent, CardDescription, CardHeader, CardTitle } from '@/components/ui/card'
import { Activity, Radio, Zap, GitBranch } from 'lucide-react'
import { api } from '@/lib/api'

interface DashboardStats {
  activeStreams: number
  gooseSubscriptions: number
  runningTests: number
  sequences: number
  backendStatus: 'connected' | 'disconnected'
  backendVersion: string
}

export function Dashboard() {
  const [stats, setStats] = useState<DashboardStats>({
    activeStreams: 0,
    gooseSubscriptions: 0,
    runningTests: 0,
    sequences: 0,
    backendStatus: 'disconnected',
    backendVersion: 'Unknown',
  })
  const [loading, setLoading] = useState(true)
  const [error, setError] = useState<string | null>(null)

  useEffect(() => {
    const fetchDashboardData = async () => {
      try {
        setLoading(true)
        setError(null)

        // Fetch all data in parallel
        const [health, streams, gooseData, sequenceStatus, analyzerStatus] = await Promise.allSettled([
          api.healthCheck(),
          api.getStreams(),
          api.getGooseSubscriptions(),
          api.getSequenceStatus(),
          api.getAnalyzerStatus(),
        ])

        // Debug logging
        console.log('[Dashboard] API responses:', {
          health: health.status === 'fulfilled' ? health.value : health.reason,
          streams: streams.status === 'fulfilled' ? streams.value : streams.reason,
          gooseData: gooseData.status === 'fulfilled' ? gooseData.value : gooseData.reason,
          sequenceStatus: sequenceStatus.status === 'fulfilled' ? sequenceStatus.value : sequenceStatus.reason,
          analyzerStatus: analyzerStatus.status === 'fulfilled' ? analyzerStatus.value : analyzerStatus.reason,
        })

        // Process health check
        const backendStatus = health.status === 'fulfilled' ? 'connected' : 'disconnected'
        const backendVersion = health.status === 'fulfilled' ? health.value.version : 'Unknown'

        // Count active streams
        const activeStreams = streams.status === 'fulfilled' 
          ? streams.value.filter(s => s.status === 'running').length 
          : 0

        // Count GOOSE subscriptions
        const gooseSubscriptions = gooseData.status === 'fulfilled' 
          ? gooseData.value.length 
          : 0

        // Count running sequences
        const runningSequences = sequenceStatus.status === 'fulfilled' && sequenceStatus.value.running 
          ? 1 
          : 0

        // Count running tests (analyzer active means test is running)
        const runningTests = analyzerStatus.status === 'fulfilled' && analyzerStatus.value.active 
          ? 1 
          : 0

        setStats({
          activeStreams,
          gooseSubscriptions,
          runningTests,
          sequences: runningSequences,
          backendStatus,
          backendVersion,
        })
      } catch (err) {
        setError(err instanceof Error ? err.message : 'Failed to fetch dashboard data')
      } finally {
        setLoading(false)
      }
    }

    fetchDashboardData()

    // Refresh every 5 seconds
    const interval = setInterval(fetchDashboardData, 5000)

    return () => clearInterval(interval)
  }, [])

  return (
    <div className="space-y-6">
      <div>
        <h1 className="text-3xl font-bold tracking-tight">Dashboard</h1>
        <p className="text-muted-foreground">
          Welcome to Virtual TestSet - IEC 61850 Sampled Values & GOOSE Testing Platform
        </p>
      </div>

      {error && (
        <div className="rounded-lg bg-destructive/10 p-4 text-sm text-destructive">
          Error: {error}
        </div>
      )}

      <div className="grid gap-4 md:grid-cols-2 lg:grid-cols-4">
        <Card>
          <CardHeader className="flex flex-row items-center justify-between space-y-0 pb-2">
            <CardTitle className="text-sm font-medium">Active Streams</CardTitle>
            <Radio className="h-4 w-4 text-muted-foreground" />
          </CardHeader>
          <CardContent>
            <div className="text-2xl font-bold">
              {loading ? '...' : stats.activeStreams}
            </div>
            <p className="text-xs text-muted-foreground">
              {stats.activeStreams === 0 ? 'No SV publishers running' : `${stats.activeStreams} running`}
            </p>
          </CardContent>
        </Card>

        <Card>
          <CardHeader className="flex flex-row items-center justify-between space-y-0 pb-2">
            <CardTitle className="text-sm font-medium">GOOSE Subscriptions</CardTitle>
            <Activity className="h-4 w-4 text-muted-foreground" />
          </CardHeader>
          <CardContent>
            <div className="text-2xl font-bold">
              {loading ? '...' : stats.gooseSubscriptions}
            </div>
            <p className="text-xs text-muted-foreground">
              {stats.gooseSubscriptions === 0 ? 'No active subscriptions' : `${stats.gooseSubscriptions} active`}
            </p>
          </CardContent>
        </Card>

        <Card>
          <CardHeader className="flex flex-row items-center justify-between space-y-0 pb-2">
            <CardTitle className="text-sm font-medium">Running Tests</CardTitle>
            <Zap className="h-4 w-4 text-muted-foreground" />
          </CardHeader>
          <CardContent>
            <div className="text-2xl font-bold">
              {loading ? '...' : stats.runningTests}
            </div>
            <p className="text-xs text-muted-foreground">
              {stats.runningTests === 0 ? 'No tests in progress' : `${stats.runningTests} running`}
            </p>
          </CardContent>
        </Card>

        <Card>
          <CardHeader className="flex flex-row items-center justify-between space-y-0 pb-2">
            <CardTitle className="text-sm font-medium">Sequences</CardTitle>
            <GitBranch className="h-4 w-4 text-muted-foreground" />
          </CardHeader>
          <CardContent>
            <div className="text-2xl font-bold">
              {loading ? '...' : stats.sequences}
            </div>
            <p className="text-xs text-muted-foreground">
              {stats.sequences === 0 ? 'No sequences running' : `${stats.sequences} running`}
            </p>
          </CardContent>
        </Card>
      </div>

      <div className="grid gap-4 md:grid-cols-2 lg:grid-cols-7">
        <Card className="col-span-4">
          <CardHeader>
            <CardTitle>Quick Start</CardTitle>
            <CardDescription>
              Get started with Virtual TestSet in three easy steps
            </CardDescription>
          </CardHeader>
          <CardContent>
            <div className="space-y-4">
              <div className="flex items-start space-x-4">
                <div className="flex h-8 w-8 items-center justify-center rounded-full bg-primary text-primary-foreground">
                  1
                </div>
                <div>
                  <h4 className="font-semibold">Create SV Publishers</h4>
                  <p className="text-sm text-muted-foreground">
                    Navigate to SV Publishers and create your first sampled value stream
                  </p>
                </div>
              </div>
              <div className="flex items-start space-x-4">
                <div className="flex h-8 w-8 items-center justify-center rounded-full bg-primary text-primary-foreground">
                  2
                </div>
                <div>
                  <h4 className="font-semibold">Inject Values</h4>
                  <p className="text-sm text-muted-foreground">
                    Use Manual Injection or COMTRADE playback to send test data
                  </p>
                </div>
              </div>
              <div className="flex items-start space-x-4">
                <div className="flex h-8 w-8 items-center justify-center rounded-full bg-primary text-primary-foreground">
                  3
                </div>
                <div>
                  <h4 className="font-semibold">Run Tests</h4>
                  <p className="text-sm text-muted-foreground">
                    Execute automated tests for distance, overcurrent, or differential relays
                  </p>
                </div>
              </div>
            </div>
          </CardContent>
        </Card>

        <Card className="col-span-3">
          <CardHeader>
            <CardTitle>System Status</CardTitle>
            <CardDescription>
              Current system information
            </CardDescription>
          </CardHeader>
          <CardContent>
            <div className="space-y-4">
              <div className="flex items-center justify-between">
                <span className="text-sm font-medium">Backend</span>
                <span className={`text-sm ${stats.backendStatus === 'connected' ? 'text-green-500' : 'text-destructive'}`}>
                  {loading ? 'Checking...' : stats.backendStatus === 'connected' ? 'Connected' : 'Disconnected'}
                </span>
              </div>
              <div className="flex items-center justify-between">
                <span className="text-sm font-medium">Backend Version</span>
                <span className="text-sm text-muted-foreground">
                  {loading ? '...' : stats.backendVersion}
                </span>
              </div>
              <div className="flex items-center justify-between">
                <span className="text-sm font-medium">Active Streams</span>
                <span className="text-sm text-muted-foreground">
                  {loading ? '...' : stats.activeStreams}
                </span>
              </div>
              <div className="flex items-center justify-between">
                <span className="text-sm font-medium">Sample Rate</span>
                <span className="text-sm text-muted-foreground">4000 Hz</span>
              </div>
            </div>
          </CardContent>
        </Card>
      </div>
    </div>
  )
}
