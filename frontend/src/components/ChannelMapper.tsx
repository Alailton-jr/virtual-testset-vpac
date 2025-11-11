import { Card, CardContent, CardDescription, CardHeader, CardTitle } from '@/components/ui/card'
import { Select, SelectContent, SelectItem, SelectTrigger, SelectValue } from '@/components/ui/select'
import { Label } from '@/components/ui/label'
import { ArrowRight } from 'lucide-react'
import type { ComtradeMetadata, ComtradeMapping } from '@/lib/types'

interface ChannelMapperProps {
  metadata: ComtradeMetadata
  mapping: ComtradeMapping[]
  onChange: (mapping: ComtradeMapping[]) => void
  disabled?: boolean
}

const SV_CHANNELS = [
  'V-A', 'V-B', 'V-C', 'V-N',
  'I-A', 'I-B', 'I-C', 'I-N'
]

export function ChannelMapper({ metadata, mapping, onChange, disabled = false }: ChannelMapperProps) {
  const updateMapping = (fileChannel: string, svChannel: string) => {
    const existingIndex = mapping.findIndex(m => m.fileChannel === fileChannel)
    
    if (existingIndex >= 0) {
      // Update existing mapping
      const newMapping = [...mapping]
      newMapping[existingIndex] = { fileChannel, svChannel }
      onChange(newMapping)
    } else {
      // Add new mapping
      onChange([...mapping, { fileChannel, svChannel }])
    }
  }

  const removeMapping = (fileChannel: string) => {
    onChange(mapping.filter(m => m.fileChannel !== fileChannel))
  }

  const getMappedSvChannel = (fileChannel: string): string => {
    const mapped = mapping.find(m => m.fileChannel === fileChannel)
    return mapped?.svChannel || ''
  }

  const getUsedSvChannels = (): Set<string> => {
    return new Set(mapping.map(m => m.svChannel))
  }

  const autoMap = () => {
    // Attempt to automatically map channels based on name patterns
    const newMapping: ComtradeMapping[] = []
    const usedSvChannels = new Set<string>()

    metadata.channels.forEach(channel => {
      const name = channel.name.toUpperCase()
      let svChannel = ''

      // Try to match voltage channels
      if (name.includes('VA') || name.includes('V_A') || name.includes('V-A') || name === 'VA') {
        svChannel = 'V-A'
      } else if (name.includes('VB') || name.includes('V_B') || name.includes('V-B') || name === 'VB') {
        svChannel = 'V-B'
      } else if (name.includes('VC') || name.includes('V_C') || name.includes('V-C') || name === 'VC') {
        svChannel = 'V-C'
      } else if (name.includes('VN') || name.includes('V_N') || name.includes('V-N') || name === 'VN') {
        svChannel = 'V-N'
      }
      // Try to match current channels
      else if (name.includes('IA') || name.includes('I_A') || name.includes('I-A') || name === 'IA') {
        svChannel = 'I-A'
      } else if (name.includes('IB') || name.includes('I_B') || name.includes('I-B') || name === 'IB') {
        svChannel = 'I-B'
      } else if (name.includes('IC') || name.includes('I_C') || name.includes('I-C') || name === 'IC') {
        svChannel = 'I-C'
      } else if (name.includes('IN') || name.includes('I_N') || name.includes('I-N') || name === 'IN') {
        svChannel = 'I-N'
      }

      // Only add if we found a match and it's not already used
      if (svChannel && !usedSvChannels.has(svChannel)) {
        newMapping.push({ fileChannel: channel.name, svChannel })
        usedSvChannels.add(svChannel)
      }
    })

    onChange(newMapping)
  }

  const usedSvChannels = getUsedSvChannels()

  return (
    <Card>
      <CardHeader>
        <div className="flex items-center justify-between">
          <div>
            <CardTitle>Channel Mapping</CardTitle>
            <CardDescription>
              Map COMTRADE file channels to SV stream channels
            </CardDescription>
          </div>
          <button
            onClick={autoMap}
            disabled={disabled}
            className="text-sm text-primary hover:underline"
          >
            Auto-map channels
          </button>
        </div>
      </CardHeader>
      <CardContent>
        <div className="space-y-3">
          {metadata.channels.map((channel, idx) => {
            const mappedChannel = getMappedSvChannel(channel.name)
            
            return (
              <div key={idx} className="flex items-center gap-4 p-3 border rounded-lg">
                {/* File Channel */}
                <div className="flex-1">
                  <Label className="text-xs text-muted-foreground">File Channel</Label>
                  <div className="flex items-center gap-2 mt-1">
                    <span className="text-xs font-mono px-2 py-1 bg-muted rounded">
                      {idx + 1}
                    </span>
                    <span className="font-medium">{channel.name}</span>
                    {channel.unit && (
                      <span className="text-xs text-muted-foreground">({channel.unit})</span>
                    )}
                  </div>
                </div>

                {/* Arrow */}
                <ArrowRight className="h-4 w-4 text-muted-foreground flex-shrink-0" />

                {/* SV Channel */}
                <div className="flex-1">
                  <Label htmlFor={`map-${idx}`} className="text-xs text-muted-foreground">
                    SV Channel
                  </Label>
                  <Select
                    value={mappedChannel}
                    onValueChange={(value) => {
                      if (value === 'none') {
                        removeMapping(channel.name)
                      } else {
                        updateMapping(channel.name, value)
                      }
                    }}
                    disabled={disabled}
                  >
                    <SelectTrigger id={`map-${idx}`} className="mt-1">
                      <SelectValue placeholder="Select channel" />
                    </SelectTrigger>
                    <SelectContent>
                      <SelectItem value="none">
                        <span className="text-muted-foreground">Not mapped</span>
                      </SelectItem>
                      {SV_CHANNELS.map(svCh => (
                        <SelectItem
                          key={svCh}
                          value={svCh}
                          disabled={usedSvChannels.has(svCh) && mappedChannel !== svCh}
                        >
                          {svCh}
                          {usedSvChannels.has(svCh) && mappedChannel !== svCh && (
                            <span className="text-xs text-muted-foreground ml-2">(in use)</span>
                          )}
                        </SelectItem>
                      ))}
                    </SelectContent>
                  </Select>
                </div>
              </div>
            )
          })}
        </div>

        {/* Mapping Summary */}
        <div className="mt-4 p-3 bg-muted rounded-lg">
          <p className="text-sm">
            <span className="font-medium">{mapping.length}</span> of{' '}
            <span className="font-medium">{metadata.channels.length}</span> channels mapped
          </p>
          {mapping.length === 0 && (
            <p className="text-xs text-muted-foreground mt-1">
              Click "Auto-map channels" or manually select SV channels
            </p>
          )}
        </div>
      </CardContent>
    </Card>
  )
}
