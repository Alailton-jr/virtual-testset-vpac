import { useEffect, useState } from 'react'
import {
  Dialog,
  DialogContent,
  DialogDescription,
  DialogFooter,
  DialogHeader,
  DialogOverlay,
  DialogTitle,
} from '@/components/ui/dialog'
import { Button } from '@/components/ui/button'
import { Input } from '@/components/ui/input'
import { Label } from '@/components/ui/label'
import type { Stream, StreamConfig } from '@/lib/types'

interface StreamConfigDialogProps {
  open: boolean
  onOpenChange: (open: boolean) => void
  stream?: Stream | null
  onSave: (config: StreamConfig) => Promise<void>
}

export function StreamConfigDialog({
  open,
  onOpenChange,
  stream,
  onSave,
}: StreamConfigDialogProps) {
  const [loading, setLoading] = useState(false)
  const [errors, setErrors] = useState<Record<string, string>>({})
  
  // Form state
  const [formData, setFormData] = useState<StreamConfig>({
    name: '',
    svID: '',
    appIdHex: '0x4000',
    macDst: '01:0C:CD:01:00:00',
    vlanId: 0,
    vlanPriority: 4,
    datSet: '',
    confRev: 1,
    smpRate: 4800,
    noASDU: 1,
    noChannels: 8,
  })

  // Load stream data when editing
  useEffect(() => {
    if (stream) {
      setFormData({
        name: stream.name,
        svID: stream.svID,
        appIdHex: stream.appIdHex,
        macDst: stream.macDst,
        vlanId: stream.vlanId,
        vlanPriority: stream.vlanPriority,
        datSet: stream.datSet,
        confRev: stream.confRev,
        smpRate: stream.smpRate,
        noASDU: stream.noASDU,
        noChannels: stream.noChannels,
      })
    } else {
      // Reset form for new stream
      setFormData({
        name: '',
        svID: '',
        appIdHex: '0x4000',
        macDst: '01:0C:CD:01:00:00',
        vlanId: 0,
        vlanPriority: 4,
        datSet: '',
        confRev: 1,
        smpRate: 4800,
        noASDU: 1,
        noChannels: 8,
      })
    }
    setErrors({})
  }, [stream, open])

  const validateForm = (): boolean => {
    const newErrors: Record<string, string> = {}

    // Name validation
    if (!formData.name.trim()) {
      newErrors.name = 'Stream name is required'
    }

    // SV ID validation
    if (!formData.svID.trim()) {
      newErrors.svID = 'SV ID is required'
    }

    // MAC address validation (format: XX:XX:XX:XX:XX:XX)
    const macPattern = /^([0-9A-Fa-f]{2}:){5}[0-9A-Fa-f]{2}$/
    if (!macPattern.test(formData.macDst)) {
      newErrors.macDst = 'Invalid MAC address format (e.g., 01:0C:CD:01:00:00)'
    }

    // App ID validation (hex format)
    const appIdPattern = /^0x[0-9A-Fa-f]{4}$/
    if (!appIdPattern.test(formData.appIdHex)) {
      newErrors.appIdHex = 'Invalid App ID format (e.g., 0x4000)'
    }

    // VLAN ID validation (0-4095)
    if (formData.vlanId < 0 || formData.vlanId > 4095) {
      newErrors.vlanId = 'VLAN ID must be between 0 and 4095'
    }

    // VLAN Priority validation (0-7)
    if (formData.vlanPriority < 0 || formData.vlanPriority > 7) {
      newErrors.vlanPriority = 'VLAN Priority must be between 0 and 7'
    }

    // Sample rate validation (common values: 4800, 9600, 14400)
    if (formData.smpRate <= 0) {
      newErrors.smpRate = 'Sample rate must be positive'
    }

    // Number of channels validation (typically 8)
    if (formData.noChannels < 1 || formData.noChannels > 32) {
      newErrors.noChannels = 'Number of channels must be between 1 and 32'
    }

    setErrors(newErrors)
    return Object.keys(newErrors).length === 0
  }

  const handleSubmit = async (e: React.FormEvent) => {
    e.preventDefault()

    if (!validateForm()) {
      return
    }

    setLoading(true)
    try {
      await onSave(formData)
      onOpenChange(false)
    } catch (error) {
      console.error('Failed to save stream:', error)
      setErrors({ submit: error instanceof Error ? error.message : 'Failed to save stream' })
    } finally {
      setLoading(false)
    }
  }

  const handleChange = (field: keyof StreamConfig, value: string | number) => {
    setFormData((prev) => ({ ...prev, [field]: value }))
    // Clear error for this field when user starts typing
    if (errors[field]) {
      setErrors((prev) => {
        const newErrors = { ...prev }
        delete newErrors[field]
        return newErrors
      })
    }
  }

  return (
    <Dialog open={open} onOpenChange={onOpenChange}>
      <DialogOverlay className="bg-black/20 dark:bg-black/20" /> 
      <DialogContent className="bg-white dark:bg-gray-900 text-gray-900 dark:text-gray-100
               border border-border max-w-2xl max-h-[90vh] overflow-y-auto">
        <DialogHeader>
          <DialogTitle>{stream ? 'Edit Stream' : 'Create New Stream'}</DialogTitle>
          <DialogDescription>
            Configure an IEC 61850-9-2 Sampled Values stream
          </DialogDescription>
        </DialogHeader>

        <form onSubmit={handleSubmit}>
          <div className="grid gap-4 py-4">
            {/* Basic Info */}
            <div className="grid grid-cols-2 gap-4">
              <div className="space-y-2">
                <Label htmlFor="name">Stream Name *</Label>
                <Input
                  id="name"
                  value={formData.name}
                  onChange={(e) => handleChange('name', e.target.value)}
                  placeholder="e.g., MU01"
                />
                {errors.name && (
                  <p className="text-sm text-red-500">{errors.name}</p>
                )}
              </div>

              <div className="space-y-2">
                <Label htmlFor="svID">SV ID *</Label>
                <Input
                  id="svID"
                  value={formData.svID}
                  onChange={(e) => handleChange('svID', e.target.value)}
                  placeholder="e.g., SV1"
                />
                {errors.svID && (
                  <p className="text-sm text-red-500">{errors.svID}</p>
                )}
              </div>
            </div>

            {/* Network Config */}
            <div className="grid grid-cols-2 gap-4">
              <div className="space-y-2">
                <Label htmlFor="appIdHex">App ID (Hex) *</Label>
                <Input
                  id="appIdHex"
                  value={formData.appIdHex}
                  onChange={(e) => handleChange('appIdHex', e.target.value)}
                  placeholder="0x4000"
                />
                {errors.appIdHex && (
                  <p className="text-sm text-red-500">{errors.appIdHex}</p>
                )}
              </div>

              <div className="space-y-2">
                <Label htmlFor="macDst">MAC Destination *</Label>
                <Input
                  id="macDst"
                  value={formData.macDst}
                  onChange={(e) => handleChange('macDst', e.target.value)}
                  placeholder="01:0C:CD:01:00:00"
                />
                {errors.macDst && (
                  <p className="text-sm text-red-500">{errors.macDst}</p>
                )}
              </div>
            </div>

            {/* VLAN Config */}
            <div className="grid grid-cols-2 gap-4">
              <div className="space-y-2">
                <Label htmlFor="vlanId">VLAN ID (0-4095)</Label>
                <Input
                  id="vlanId"
                  type="number"
                  min="0"
                  max="4095"
                  value={formData.vlanId}
                  onChange={(e) => handleChange('vlanId', parseInt(e.target.value) || 0)}
                />
                {errors.vlanId && (
                  <p className="text-sm text-red-500">{errors.vlanId}</p>
                )}
              </div>

              <div className="space-y-2">
                <Label htmlFor="vlanPriority">VLAN Priority (0-7)</Label>
                <Input
                  id="vlanPriority"
                  type="number"
                  min="0"
                  max="7"
                  value={formData.vlanPriority}
                  onChange={(e) => handleChange('vlanPriority', parseInt(e.target.value) || 4)}
                />
                {errors.vlanPriority && (
                  <p className="text-sm text-red-500">{errors.vlanPriority}</p>
                )}
              </div>
            </div>

            {/* Dataset Config */}
            <div className="grid grid-cols-2 gap-4">
              <div className="space-y-2">
                <Label htmlFor="datSet">Dataset Name</Label>
                <Input
                  id="datSet"
                  value={formData.datSet}
                  onChange={(e) => handleChange('datSet', e.target.value)}
                  placeholder="e.g., DS1"
                />
              </div>

              <div className="space-y-2">
                <Label htmlFor="confRev">Config Revision</Label>
                <Input
                  id="confRev"
                  type="number"
                  min="1"
                  value={formData.confRev}
                  onChange={(e) => handleChange('confRev', parseInt(e.target.value) || 1)}
                />
              </div>
            </div>

            {/* Sample Config */}
            <div className="grid grid-cols-3 gap-4">
              <div className="space-y-2">
                <Label htmlFor="smpRate">Sample Rate (Hz) *</Label>
                <Input
                  id="smpRate"
                  type="number"
                  min="1"
                  value={formData.smpRate}
                  onChange={(e) => handleChange('smpRate', parseInt(e.target.value) || 4800)}
                  placeholder="4800"
                />
                {errors.smpRate && (
                  <p className="text-sm text-red-500">{errors.smpRate}</p>
                )}
              </div>

              <div className="space-y-2">
                <Label htmlFor="noASDU">No. ASDU</Label>
                <Input
                  id="noASDU"
                  type="number"
                  min="1"
                  value={formData.noASDU}
                  onChange={(e) => handleChange('noASDU', parseInt(e.target.value) || 1)}
                />
              </div>

              <div className="space-y-2">
                <Label htmlFor="noChannels">No. Channels *</Label>
                <Input
                  id="noChannels"
                  type="number"
                  min="1"
                  max="32"
                  value={formData.noChannels}
                  onChange={(e) => handleChange('noChannels', parseInt(e.target.value) || 8)}
                />
                {errors.noChannels && (
                  <p className="text-sm text-red-500">{errors.noChannels}</p>
                )}
              </div>
            </div>

            {errors.submit && (
              <div className="rounded-md bg-red-50 dark:bg-red-900/20 p-3">
                <p className="text-sm text-red-600 dark:text-red-400">{errors.submit}</p>
              </div>
            )}
          </div>

          <DialogFooter>
            <Button
              type="button"
              variant="outline"
              onClick={() => onOpenChange(false)}
              disabled={loading}
            >
              Cancel
            </Button>
            <Button type="submit" disabled={loading}>
              {loading ? 'Saving...' : stream ? 'Update Stream' : 'Create Stream'}
            </Button>
          </DialogFooter>
        </form>
      </DialogContent>
    </Dialog>
  )
}
