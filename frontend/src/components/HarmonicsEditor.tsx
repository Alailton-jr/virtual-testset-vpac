import { useState } from 'react'
import { Button } from '@/components/ui/button'
import { Card, CardContent, CardDescription, CardHeader, CardTitle } from '@/components/ui/card'
import { Input } from '@/components/ui/input'
import { Label } from '@/components/ui/label'
import { Select, SelectContent, SelectItem, SelectTrigger, SelectValue } from '@/components/ui/select'
import { Slider } from '@/components/ui/slider'
import { Loader2, Plus, Trash2 } from 'lucide-react'

export interface HarmonicData {
  n: number         // Harmonic order (1, 2, 3, 5, 7, 11, 13, etc.)
  mag: number       // Magnitude (percentage of fundamental)
  angleDeg: number  // Phase angle in degrees
}

export interface HarmonicsValues {
  channel: string
  harmonics: HarmonicData[]
}

interface HarmonicsEditorProps {
  onApply: (values: HarmonicsValues) => Promise<void>
  disabled?: boolean
}

const ALL_CHANNELS = ['V-A', 'V-B', 'V-C', 'V-N', 'I-A', 'I-B', 'I-C', 'I-N']
const COMMON_HARMONICS = [1, 2, 3, 5, 7, 11, 13, 17, 19, 23, 25, 29, 31]

// Default: fundamental only (100% magnitude, 0 degrees)
const DEFAULT_VALUES: HarmonicsValues = {
  channel: 'V-A',
  harmonics: [
    { n: 1, mag: 100, angleDeg: 0 },
  ],
}

export function HarmonicsEditor({ onApply, disabled = false }: HarmonicsEditorProps) {
  const [values, setValues] = useState<HarmonicsValues>(DEFAULT_VALUES)
  const [isLoading, setIsLoading] = useState(false)
  const [errors, setErrors] = useState<{ [key: string]: string }>({})

  const updateChannel = (channel: string) => {
    setValues(prev => ({ ...prev, channel }))
  }

  const updateHarmonic = (index: number, field: keyof HarmonicData, value: number) => {
    setValues(prev => ({
      ...prev,
      harmonics: prev.harmonics.map((h, i) => 
        i === index ? { ...h, [field]: value } : h
      ),
    }))
    // Clear error when user edits
    if (errors[`harmonic-${index}-${field}`]) {
      setErrors(prev => {
        const newErrors = { ...prev }
        delete newErrors[`harmonic-${index}-${field}`]
        return newErrors
      })
    }
  }

  const addHarmonic = () => {
    // Find next available harmonic order
    const usedOrders = new Set(values.harmonics.map(h => h.n))
    const nextOrder = COMMON_HARMONICS.find(n => !usedOrders.has(n)) || 
                      values.harmonics.length > 0 ? Math.max(...values.harmonics.map(h => h.n)) + 2 : 3

    setValues(prev => ({
      ...prev,
      harmonics: [...prev.harmonics, { n: nextOrder, mag: 0, angleDeg: 0 }].sort((a, b) => a.n - b.n),
    }))
  }

  const removeHarmonic = (index: number) => {
    setValues(prev => ({
      ...prev,
      harmonics: prev.harmonics.filter((_, i) => i !== index),
    }))
  }

  const validateValues = (): boolean => {
    const newErrors: { [key: string]: string } = {}

    // Validate each harmonic
    values.harmonics.forEach((harmonic, index) => {
      // Validate harmonic order (1-50)
      if (harmonic.n < 1 || harmonic.n > 50) {
        newErrors[`harmonic-${index}-n`] = 'Harmonic order must be between 1 and 50'
      }

      // Validate magnitude (0-100%)
      if (harmonic.mag < 0 || harmonic.mag > 100) {
        newErrors[`harmonic-${index}-mag`] = 'Magnitude must be between 0 and 100%'
      }

      // Validate angle (-360 to 360 degrees)
      if (harmonic.angleDeg < -360 || harmonic.angleDeg > 360) {
        newErrors[`harmonic-${index}-angleDeg`] = 'Angle must be between -360 and 360 degrees'
      }
    })

    // Check for duplicate harmonic orders
    const orders = values.harmonics.map(h => h.n)
    const duplicates = orders.filter((n, i) => orders.indexOf(n) !== i)
    if (duplicates.length > 0) {
      duplicates.forEach(n => {
        const index = orders.indexOf(n)
        newErrors[`harmonic-${index}-n`] = `Duplicate harmonic order ${n}`
      })
    }

    setErrors(newErrors)
    return Object.keys(newErrors).length === 0
  }

  const handleApply = async () => {
    if (!validateValues()) {
      return
    }

    setIsLoading(true)
    try {
      await onApply(values)
    } finally {
      setIsLoading(false)
    }
  }

  const handleReset = () => {
    setValues(DEFAULT_VALUES)
    setErrors({})
  }

  const renderHarmonicControl = (harmonic: HarmonicData, index: number) => {
    return (
      <Card key={index}>
        <CardHeader className="pb-3">
          <div className="flex items-center justify-between">
            <div>
              <CardTitle className="text-sm font-medium">
                Harmonic {harmonic.n}
              </CardTitle>
              <CardDescription>
                {harmonic.mag.toFixed(1)}% ∠ {harmonic.angleDeg.toFixed(1)}°
              </CardDescription>
            </div>
            <Button
              variant="ghost"
              size="sm"
              onClick={() => removeHarmonic(index)}
              disabled={disabled || isLoading || values.harmonics.length === 1}
              className="h-8 w-8 p-0"
            >
              <Trash2 className="h-4 w-4" />
            </Button>
          </div>
        </CardHeader>
        <CardContent className="space-y-4">
          {/* Harmonic Order */}
          <div className="space-y-2">
            <Label htmlFor={`harmonic-${index}-order`} className="text-xs">
              Order
            </Label>
            <Input
              id={`harmonic-${index}-order`}
              type="number"
              value={harmonic.n}
              onChange={(e) => {
                const value = parseInt(e.target.value) || 1
                updateHarmonic(index, 'n', value)
              }}
              disabled={disabled || isLoading}
              className="h-8"
              min={1}
              max={50}
            />
            {errors[`harmonic-${index}-n`] && (
              <p className="text-xs text-red-500">{errors[`harmonic-${index}-n`]}</p>
            )}
          </div>

          {/* Magnitude Control */}
          <div className="space-y-2">
            <Label htmlFor={`harmonic-${index}-mag`} className="text-xs">
              Magnitude (% of fundamental)
            </Label>
            <div className="flex gap-2 items-center">
              <Slider
                id={`harmonic-${index}-mag`}
                value={[harmonic.mag]}
                onValueChange={([value]) => updateHarmonic(index, 'mag', value)}
                min={0}
                max={100}
                step={0.1}
                disabled={disabled || isLoading}
                className="flex-1"
              />
              <Input
                type="number"
                value={harmonic.mag.toFixed(1)}
                onChange={(e) => {
                  const value = parseFloat(e.target.value) || 0
                  updateHarmonic(index, 'mag', value)
                }}
                disabled={disabled || isLoading}
                className="w-20 h-8 text-xs"
                step={0.1}
              />
            </div>
            {errors[`harmonic-${index}-mag`] && (
              <p className="text-xs text-red-500">{errors[`harmonic-${index}-mag`]}</p>
            )}
          </div>

          {/* Angle Control */}
          <div className="space-y-2">
            <Label htmlFor={`harmonic-${index}-angle`} className="text-xs">
              Angle (degrees)
            </Label>
            <div className="flex gap-2 items-center">
              <Slider
                id={`harmonic-${index}-angle`}
                value={[harmonic.angleDeg]}
                onValueChange={([value]) => updateHarmonic(index, 'angleDeg', value)}
                min={-180}
                max={180}
                step={1}
                disabled={disabled || isLoading}
                className="flex-1"
              />
              <Input
                type="number"
                value={harmonic.angleDeg.toFixed(1)}
                onChange={(e) => {
                  const value = parseFloat(e.target.value) || 0
                  updateHarmonic(index, 'angleDeg', value)
                }}
                disabled={disabled || isLoading}
                className="w-20 h-8 text-xs"
                step={1}
              />
            </div>
            {errors[`harmonic-${index}-angleDeg`] && (
              <p className="text-xs text-red-500">{errors[`harmonic-${index}-angleDeg`]}</p>
            )}
          </div>
        </CardContent>
      </Card>
    )
  }

  return (
    <div className="space-y-6">
      {/* Channel Selection */}
      <Card>
        <CardHeader>
          <CardTitle>Channel Selection</CardTitle>
          <CardDescription>Select the channel to configure harmonics for</CardDescription>
        </CardHeader>
        <CardContent>
          <div className="space-y-2">
            <Label htmlFor="channel">Channel</Label>
            <Select value={values.channel} onValueChange={updateChannel} disabled={disabled || isLoading}>
              <SelectTrigger id="channel">
                <SelectValue />
              </SelectTrigger>
              <SelectContent>
                {ALL_CHANNELS.map(channel => (
                  <SelectItem key={channel} value={channel}>
                    {channel}
                  </SelectItem>
                ))}
              </SelectContent>
            </Select>
          </div>
        </CardContent>
      </Card>

      {/* Harmonics List */}
      <div>
        <div className="flex items-center justify-between mb-3">
          <h3 className="text-lg font-semibold">Harmonics</h3>
          <Button
            variant="outline"
            size="sm"
            onClick={addHarmonic}
            disabled={disabled || isLoading}
          >
            <Plus className="mr-2 h-4 w-4" />
            Add Harmonic
          </Button>
        </div>
        <div className="grid grid-cols-1 md:grid-cols-2 lg:grid-cols-3 gap-4">
          {values.harmonics.map((harmonic, index) => renderHarmonicControl(harmonic, index))}
        </div>
      </div>

      {/* Info Card */}
      <Card className="border-blue-200 bg-blue-50">
        <CardHeader className="pb-3">
          <CardTitle className="text-sm">Harmonic Configuration Guide</CardTitle>
        </CardHeader>
        <CardContent className="text-sm text-muted-foreground space-y-1">
          <p>• <strong>Order 1:</strong> Fundamental frequency (usually 100%)</p>
          <p>• <strong>Orders 3, 5, 7, 11, 13:</strong> Common odd harmonics</p>
          <p>• <strong>Orders 2, 4, 6, 8:</strong> Even harmonics (less common)</p>
          <p>• <strong>Magnitude:</strong> Percentage relative to fundamental</p>
          <p>• <strong>Angle:</strong> Phase shift from fundamental</p>
        </CardContent>
      </Card>

      {/* Action Buttons */}
      <div className="flex gap-2 justify-end">
        <Button
          variant="outline"
          onClick={handleReset}
          disabled={disabled || isLoading}
        >
          Reset to Defaults
        </Button>
        <Button onClick={handleApply} disabled={disabled || isLoading}>
          {isLoading && <Loader2 className="mr-2 h-4 w-4 animate-spin" />}
          Apply Harmonics
        </Button>
      </div>
    </div>
  )
}
