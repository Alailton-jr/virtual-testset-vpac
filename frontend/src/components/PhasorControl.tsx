import { useState } from 'react'
import { Button } from '@/components/ui/button'
import { Card, CardContent, CardDescription, CardHeader, CardTitle } from '@/components/ui/card'
import { Input } from '@/components/ui/input'
import { Label } from '@/components/ui/label'
import { Slider } from '@/components/ui/slider'
import { Loader2 } from 'lucide-react'

export interface PhasorValues {
  freq: number
  channels: {
    'V-A': { mag: number; angleDeg: number }
    'V-B': { mag: number; angleDeg: number }
    'V-C': { mag: number; angleDeg: number }
    'V-N': { mag: number; angleDeg: number }
    'I-A': { mag: number; angleDeg: number }
    'I-B': { mag: number; angleDeg: number }
    'I-C': { mag: number; angleDeg: number }
    'I-N': { mag: number; angleDeg: number }
  }
}

interface PhasorControlProps {
  onApply: (values: PhasorValues) => Promise<void>
  disabled?: boolean
}

const VOLTAGE_CHANNELS = ['V-A', 'V-B', 'V-C', 'V-N'] as const
const CURRENT_CHANNELS = ['I-A', 'I-B', 'I-C', 'I-N'] as const
const ALL_CHANNELS = [...VOLTAGE_CHANNELS, ...CURRENT_CHANNELS]

type ChannelName = 'V-A' | 'V-B' | 'V-C' | 'V-N' | 'I-A' | 'I-B' | 'I-C' | 'I-N'

// Reasonable defaults for 60Hz system
const DEFAULT_VALUES: PhasorValues = {
  freq: 60,
  channels: {
    'V-A': { mag: 69000 / Math.sqrt(3), angleDeg: 0 },      // Phase A voltage
    'V-B': { mag: 69000 / Math.sqrt(3), angleDeg: -120 },   // Phase B voltage
    'V-C': { mag: 69000 / Math.sqrt(3), angleDeg: 120 },    // Phase C voltage
    'V-N': { mag: 0, angleDeg: 0 },                         // Neutral voltage
    'I-A': { mag: 1000, angleDeg: -30 },                    // Phase A current
    'I-B': { mag: 1000, angleDeg: -150 },                   // Phase B current
    'I-C': { mag: 1000, angleDeg: 90 },                     // Phase C current
    'I-N': { mag: 0, angleDeg: 0 },                         // Neutral current
  },
}

export function PhasorControl({ onApply, disabled = false }: PhasorControlProps) {
  const [values, setValues] = useState<PhasorValues>(DEFAULT_VALUES)
  const [isLoading, setIsLoading] = useState(false)
  const [errors, setErrors] = useState<{ [key: string]: string }>({})

  const updateChannel = (channel: ChannelName, field: 'mag' | 'angleDeg', value: number) => {
    setValues(prev => ({
      ...prev,
      channels: {
        ...prev.channels,
        [channel]: {
          ...prev.channels[channel],
          [field]: value,
        },
      },
    }))
    // Clear error when user edits
    if (errors[`${channel}-${field}`]) {
      setErrors(prev => {
        const newErrors = { ...prev }
        delete newErrors[`${channel}-${field}`]
        return newErrors
      })
    }
  }

  const updateFrequency = (freq: number) => {
    setValues(prev => ({ ...prev, freq }))
    if (errors.freq) {
      setErrors(prev => {
        const newErrors = { ...prev }
        delete newErrors.freq
        return newErrors
      })
    }
  }

  const validateValues = (): boolean => {
    const newErrors: { [key: string]: string } = {}

    // Validate frequency (50-60 Hz typical)
    if (values.freq < 45 || values.freq > 65) {
      newErrors.freq = 'Frequency must be between 45 and 65 Hz'
    }

    // Validate voltage channels (0-500 kV)
    VOLTAGE_CHANNELS.forEach(channel => {
      const mag = values.channels[channel as ChannelName].mag
      if (mag < 0 || mag > 500000) {
        newErrors[`${channel}-mag`] = 'Voltage must be between 0 and 500 kV'
      }
    })

    // Validate current channels (0-50 kA)
    CURRENT_CHANNELS.forEach(channel => {
      const mag = values.channels[channel as ChannelName].mag
      if (mag < 0 || mag > 50000) {
        newErrors[`${channel}-mag`] = 'Current must be between 0 and 50 kA'
      }
    })

    // Validate angles (0-360 degrees)
    ALL_CHANNELS.forEach(channel => {
      const angle = values.channels[channel as ChannelName].angleDeg
      if (angle < -360 || angle > 360) {
        newErrors[`${channel}-angleDeg`] = 'Angle must be between -360 and 360 degrees'
      }
    })

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

  const renderChannelControl = (channel: ChannelName, isVoltage: boolean) => {
    const channelData = values.channels[channel]
    const unit = isVoltage ? 'V' : 'A'
    const displayMag = isVoltage ? channelData.mag / 1000 : channelData.mag // Display in kV or A

    return (
      <Card key={channel}>
        <CardHeader className="pb-3">
          <CardTitle className="text-sm font-medium">{channel}</CardTitle>
          <CardDescription>
            {displayMag.toFixed(2)} k{unit} ∠ {channelData.angleDeg.toFixed(1)}°
          </CardDescription>
        </CardHeader>
        <CardContent className="space-y-4">
          {/* Magnitude Control */}
          <div className="space-y-2">
            <Label htmlFor={`${channel}-mag`} className="text-xs">
              Magnitude (k{unit})
            </Label>
            <div className="flex gap-2 items-center">
              <Slider
                id={`${channel}-mag`}
                value={[displayMag]}
                onValueChange={([value]) =>
                  updateChannel(channel as ChannelName, 'mag', isVoltage ? value * 1000 : value)
                }
                min={0}
                max={isVoltage ? 500 : 50}
                step={isVoltage ? 1 : 0.1}
                disabled={disabled || isLoading}
                className="flex-1"
              />
              <Input
                type="number"
                value={displayMag.toFixed(2)}
                onChange={(e) => {
                  const value = parseFloat(e.target.value) || 0
                  updateChannel(channel as ChannelName, 'mag', isVoltage ? value * 1000 : value)
                }}
                disabled={disabled || isLoading}
                className="w-20 h-8 text-xs"
                step={isVoltage ? 1 : 0.1}
              />
            </div>
            {errors[`${channel}-mag`] && (
              <p className="text-xs text-red-500">{errors[`${channel}-mag`]}</p>
            )}
          </div>

          {/* Angle Control */}
          <div className="space-y-2">
            <Label htmlFor={`${channel}-angle`} className="text-xs">
              Angle (degrees)
            </Label>
            <div className="flex gap-2 items-center">
              <Slider
                id={`${channel}-angle`}
                value={[channelData.angleDeg]}
                onValueChange={([value]) => updateChannel(channel as ChannelName, 'angleDeg', value)}
                min={-180}
                max={180}
                step={1}
                disabled={disabled || isLoading}
                className="flex-1"
              />
              <Input
                type="number"
                value={channelData.angleDeg.toFixed(1)}
                onChange={(e) => {
                  const value = parseFloat(e.target.value) || 0
                  updateChannel(channel as ChannelName, 'angleDeg', value)
                }}
                disabled={disabled || isLoading}
                className="w-20 h-8 text-xs"
                step={1}
              />
            </div>
            {errors[`${channel}-angleDeg`] && (
              <p className="text-xs text-red-500">{errors[`${channel}-angleDeg`]}</p>
            )}
          </div>
        </CardContent>
      </Card>
    )
  }

  return (
    <div className="space-y-6">
      {/* Frequency Control */}
      <Card>
        <CardHeader>
          <CardTitle>System Frequency</CardTitle>
          <CardDescription>Set the fundamental frequency for all channels</CardDescription>
        </CardHeader>
        <CardContent className="space-y-4">
          <div className="space-y-2">
            <Label htmlFor="frequency">Frequency (Hz)</Label>
            <div className="flex gap-2 items-center">
              <Slider
                id="frequency"
                value={[values.freq]}
                onValueChange={([value]) => updateFrequency(value)}
                min={45}
                max={65}
                step={0.01}
                disabled={disabled || isLoading}
                className="flex-1"
              />
              <Input
                type="number"
                value={values.freq.toFixed(2)}
                onChange={(e) => {
                  const value = parseFloat(e.target.value) || 60
                  updateFrequency(value)
                }}
                disabled={disabled || isLoading}
                className="w-24"
                step={0.01}
              />
            </div>
            {errors.freq && <p className="text-sm text-red-500">{errors.freq}</p>}
          </div>
        </CardContent>
      </Card>

      {/* Voltage Channels */}
      <div>
        <h3 className="text-lg font-semibold mb-3">Voltage Channels</h3>
        <div className="grid grid-cols-1 md:grid-cols-2 lg:grid-cols-4 gap-4">
          {VOLTAGE_CHANNELS.map(channel => renderChannelControl(channel, true))}
        </div>
      </div>

      {/* Current Channels */}
      <div>
        <h3 className="text-lg font-semibold mb-3">Current Channels</h3>
        <div className="grid grid-cols-1 md:grid-cols-2 lg:grid-cols-4 gap-4">
          {CURRENT_CHANNELS.map(channel => renderChannelControl(channel, false))}
        </div>
      </div>

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
          Apply Phasors
        </Button>
      </div>
    </div>
  )
}
