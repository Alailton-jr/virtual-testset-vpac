import { useState, useEffect } from 'react'
import { Card, CardContent, CardDescription, CardHeader, CardTitle } from '@/components/ui/card'
import { Button } from '@/components/ui/button'
import { Input } from '@/components/ui/input'
import { Label } from '@/components/ui/label'
import { Select, SelectContent, SelectItem, SelectTrigger, SelectValue } from '@/components/ui/select'
import { Alert, AlertDescription } from '@/components/ui/alert'
import { AlertCircle, Zap, ZapOff } from 'lucide-react'
import { useStreamStore } from '@/stores/useStreamStore'
import { api } from '@/lib/api'
import type { ImpedanceConfig } from '@/lib/types'

export default function ImpedancePage() {
  const { streams, fetchStreams } = useStreamStore()
  const [selectedStreamId, setSelectedStreamId] = useState<string>('')
  const [faultType, setFaultType] = useState<ImpedanceConfig['faultType']>('AG')
  const [R, setR] = useState<number>(5.0)
  const [X, setX] = useState<number>(10.0)
  const [RS1, setRS1] = useState<number>(0.5)
  const [XS1, setXS1] = useState<number>(5.0)
  const [RS0, setRS0] = useState<number>(1.0)
  const [XS0, setXS0] = useState<number>(10.0)
  const [Vprefault, setVprefault] = useState<number>(115.47)
  const [isActive, setIsActive] = useState(false)
  const [error, setError] = useState<string>('')

  useEffect(() => { fetchStreams() }, [fetchStreams])
  useEffect(() => { if (error) { const timer = setTimeout(() => setError(''), 5000); return () => clearTimeout(timer) } }, [error])

  const handleApply = async () => {
    if (!selectedStreamId) {
      setError('Please select a stream')
      return
    }

    setError('')
    try {
      const config: ImpedanceConfig = {
        streamId: selectedStreamId,
        R,
        X,
        faultType,
        source: {
          RS1,
          XS1,
          RS0,
          XS0,
          Vprefault
        }
      }
      await api.applyImpedance(config)
      setIsActive(true)
    } catch (err) {
      setError(err instanceof Error ? err.message : 'Failed to apply impedance')
    }
  }

  const handleClear = async () => {
    if (!selectedStreamId) return

    try {
      await api.clearImpedance(selectedStreamId)
      setIsActive(false)
      setError('')
    } catch (err) {
      setError(err instanceof Error ? err.message : 'Failed to clear impedance')
    }
  }

  return (
    <div className="space-y-6">
      <div>
        <h1 className="text-3xl font-bold tracking-tight">Impedance Injection</h1>
        <p className="text-muted-foreground">
          Inject fault impedance for protection testing
        </p>
      </div>

      {error && (
        <Alert variant="destructive">
          <AlertCircle className="h-4 w-4" />
          <AlertDescription>{error}</AlertDescription>
        </Alert>
      )}

      {isActive && (
        <Alert>
          <Zap className="h-4 w-4" />
          <AlertDescription>
            Impedance injection is currently active on the selected stream
          </AlertDescription>
        </Alert>
      )}

      <Card>
        <CardHeader>
          <CardTitle>Stream Selection</CardTitle>
          <CardDescription>Select the target stream for impedance injection</CardDescription>
        </CardHeader>
        <CardContent>
          <div className="space-y-2">
            <Label>Target Stream</Label>
            <Select value={selectedStreamId} onValueChange={setSelectedStreamId} disabled={isActive}>
              <SelectTrigger>
                <SelectValue placeholder="Select stream" />
              </SelectTrigger>
              <SelectContent>
                {streams.map(stream => (
                  <SelectItem key={stream.id} value={stream.id}>
                    {stream.name}
                  </SelectItem>
                ))}
              </SelectContent>
            </Select>
          </div>
        </CardContent>
      </Card>

      <Card>
        <CardHeader>
          <CardTitle>Fault Configuration</CardTitle>
          <CardDescription>
            Configure fault type and impedance (R+jX)
          </CardDescription>
        </CardHeader>
        <CardContent className="space-y-4">
          <div className="grid grid-cols-2 gap-4">
            <div className="space-y-2">
              <Label>Fault Type</Label>
              <Select 
                value={faultType} 
                onValueChange={(val) => setFaultType(val as ImpedanceConfig['faultType'])}
                disabled={isActive}
              >
                <SelectTrigger>
                  <SelectValue />
                </SelectTrigger>
                <SelectContent>
                  <SelectItem value="AG">A-G (Phase A to Ground)</SelectItem>
                  <SelectItem value="BG">B-G (Phase B to Ground)</SelectItem>
                  <SelectItem value="CG">C-G (Phase C to Ground)</SelectItem>
                  <SelectItem value="AB">A-B (Phase to Phase)</SelectItem>
                  <SelectItem value="BC">B-C (Phase to Phase)</SelectItem>
                  <SelectItem value="CA">C-A (Phase to Phase)</SelectItem>
                  <SelectItem value="ABC">ABC (Three Phase)</SelectItem>
                </SelectContent>
              </Select>
            </div>
            <div className="space-y-2">
              <Label>Prefault Voltage (kV L-N)</Label>
              <Input
                type="number"
                value={Vprefault}
                onChange={(e) => setVprefault(parseFloat(e.target.value) || 0)}
                disabled={isActive}
                step={0.1}
              />
            </div>
          </div>

          <div className="grid grid-cols-2 gap-4">
            <div className="space-y-2">
              <Label>Fault Resistance R (Ω)</Label>
              <Input
                type="number"
                value={R}
                onChange={(e) => setR(parseFloat(e.target.value) || 0)}
                disabled={isActive}
                step={0.1}
              />
            </div>
            <div className="space-y-2">
              <Label>Fault Reactance X (Ω)</Label>
              <Input
                type="number"
                value={X}
                onChange={(e) => setX(parseFloat(e.target.value) || 0)}
                disabled={isActive}
                step={0.1}
              />
            </div>
          </div>
        </CardContent>
      </Card>

      <Card>
        <CardHeader>
          <CardTitle>Source Impedance</CardTitle>
          <CardDescription>
            Positive and zero sequence source impedances
          </CardDescription>
        </CardHeader>
        <CardContent className="space-y-4">
          <div className="space-y-2">
            <h3 className="text-sm font-semibold">Positive Sequence (Z1)</h3>
            <div className="grid grid-cols-2 gap-4">
              <div className="space-y-2">
                <Label>RS1 (Ω)</Label>
                <Input
                  type="number"
                  value={RS1}
                  onChange={(e) => setRS1(parseFloat(e.target.value) || 0)}
                  disabled={isActive}
                  step={0.1}
                />
              </div>
              <div className="space-y-2">
                <Label>XS1 (Ω)</Label>
                <Input
                  type="number"
                  value={XS1}
                  onChange={(e) => setXS1(parseFloat(e.target.value) || 0)}
                  disabled={isActive}
                  step={0.1}
                />
              </div>
            </div>
          </div>

          <div className="space-y-2">
            <h3 className="text-sm font-semibold">Zero Sequence (Z0)</h3>
            <div className="grid grid-cols-2 gap-4">
              <div className="space-y-2">
                <Label>RS0 (Ω)</Label>
                <Input
                  type="number"
                  value={RS0}
                  onChange={(e) => setRS0(parseFloat(e.target.value) || 0)}
                  disabled={isActive}
                  step={0.1}
                />
              </div>
              <div className="space-y-2">
                <Label>XS0 (Ω)</Label>
                <Input
                  type="number"
                  value={XS0}
                  onChange={(e) => setXS0(parseFloat(e.target.value) || 0)}
                  disabled={isActive}
                  step={0.1}
                />
              </div>
            </div>
          </div>
        </CardContent>
      </Card>

      <Card>
        <CardHeader>
          <CardTitle>Control</CardTitle>
          <CardDescription>Apply or clear impedance injection</CardDescription>
        </CardHeader>
        <CardContent className="space-y-4">
          <div className="grid grid-cols-2 gap-4">
            <Button 
              onClick={handleApply} 
              disabled={isActive || !selectedStreamId}
              className="w-full"
            >
              <Zap className="mr-2 h-4 w-4" />
              Apply Impedance
            </Button>
            <Button 
              onClick={handleClear} 
              disabled={!isActive}
              variant="destructive"
              className="w-full"
            >
              <ZapOff className="mr-2 h-4 w-4" />
              Clear Impedance
            </Button>
          </div>

          <div className="p-3 bg-muted/20 rounded-md text-sm text-muted-foreground space-y-1">
            <p><strong>Current Configuration:</strong></p>
            <p>• Fault: {faultType}</p>
            <p>• Impedance: {R.toFixed(2)} + j{X.toFixed(2)} Ω</p>
            <p>• Z1 Source: {RS1.toFixed(2)} + j{XS1.toFixed(2)} Ω</p>
            <p>• Z0 Source: {RS0.toFixed(2)} + j{XS0.toFixed(2)} Ω</p>
            <p>• Vprefault: {Vprefault.toFixed(2)} kV</p>
          </div>
        </CardContent>
      </Card>
    </div>
  )
}
