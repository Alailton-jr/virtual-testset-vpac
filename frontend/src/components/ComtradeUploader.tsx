import { useState, useRef } from 'react'
import { Button } from '@/components/ui/button'
import { Card, CardContent, CardDescription, CardHeader, CardTitle } from '@/components/ui/card'
import { Alert, AlertDescription } from '@/components/ui/alert'
import { Upload, FileText, AlertCircle, X } from 'lucide-react'
import type { ComtradeMetadata } from '@/lib/types'

interface ComtradeUploaderProps {
  onUpload: (file: File, metadata: ComtradeMetadata) => void
  onClear: () => void
  disabled?: boolean
}

export function ComtradeUploader({ onUpload, onClear, disabled = false }: ComtradeUploaderProps) {
  const [file, setFile] = useState<File | null>(null)
  const [metadata, setMetadata] = useState<ComtradeMetadata | null>(null)
  const [error, setError] = useState<string>('')
  const [isProcessing, setIsProcessing] = useState(false)
  const fileInputRef = useRef<HTMLInputElement>(null)

  const parseComtradeMetadata = async (file: File): Promise<ComtradeMetadata> => {
    // For now, we'll do a simple client-side parse of basic metadata
    // In production, this would call the backend API for full parsing
    
    const text = await file.text()
    const lines = text.split('\n')
    
    // Detect file type
    const isCSV = file.name.endsWith('.csv')
    const isCfg = file.name.endsWith('.cfg')
    
    if (isCSV) {
      // Simple CSV parsing - first line is headers
      const headers = lines[0].split(',').map(h => h.trim())
      const dataLines = lines.slice(1).filter(l => l.trim())
      
      return {
        filename: file.name,
        sampleRate: 4800, // Default assumption
        totalSamples: dataLines.length,
        channels: headers.slice(1).map((name, idx) => ({ // Skip time column
          name: name || `Channel${idx + 1}`,
          type: 'analog',
          unit: name.includes('V') || name.includes('Voltage') ? 'V' : 
                name.includes('I') || name.includes('Current') ? 'A' : '',
        })),
      }
    } else if (isCfg) {
      // Parse COMTRADE .cfg file
      // Line 1: station_name, rec_dev_id, rev_year
      // Line 2: TT, ##A, ##D
      // Line 3+: channel definitions
      
      // Line 2: TT, ##A, ##D
      
      const line2 = lines[1]?.split(',') || []
      const analogChannels = parseInt(line2[1]?.replace('A', '')) || 0
      
      const channels = []
      for (let i = 0; i < analogChannels && i < lines.length - 2; i++) {
        const channelLine = lines[2 + i]?.split(',') || []
        channels.push({
          name: channelLine[1]?.trim() || `Channel${i + 1}`,
          type: 'analog' as const,
          unit: channelLine[3]?.trim() || '',
        })
      }
      
      // Sample rate from line with sampling info
      const samplingLine = lines.find(l => l.includes('nrates') || l.includes(','))
      let sampleRate = 4800 // default
      
      if (samplingLine) {
        const parts = samplingLine.split(',')
        if (parts.length > 1) {
          sampleRate = parseFloat(parts[1]) || 4800
        }
      }
      
      return {
        filename: file.name,
        sampleRate,
        totalSamples: 0, // Will be determined from .dat file
        channels,
      }
    } else {
      throw new Error('Unsupported file format. Please upload .cfg, .dat, or .csv files.')
    }
  }

  const handleFileSelect = async (event: React.ChangeEvent<HTMLInputElement>) => {
    const selectedFile = event.target.files?.[0]
    if (!selectedFile) return

    setIsProcessing(true)
    setError('')

    try {
      const parsedMetadata = await parseComtradeMetadata(selectedFile)
      setFile(selectedFile)
      setMetadata(parsedMetadata)
      onUpload(selectedFile, parsedMetadata)
    } catch (err) {
      setError(err instanceof Error ? err.message : 'Failed to parse file')
      setFile(null)
      setMetadata(null)
    } finally {
      setIsProcessing(false)
    }
  }

  const handleClear = () => {
    setFile(null)
    setMetadata(null)
    setError('')
    if (fileInputRef.current) {
      fileInputRef.current.value = ''
    }
    onClear()
  }

  const handleDrop = (event: React.DragEvent) => {
    event.preventDefault()
    const droppedFile = event.dataTransfer.files?.[0]
    if (droppedFile && fileInputRef.current) {
      const dataTransfer = new DataTransfer()
      dataTransfer.items.add(droppedFile)
      fileInputRef.current.files = dataTransfer.files
      fileInputRef.current.dispatchEvent(new Event('change', { bubbles: true }))
    }
  }

  const handleDragOver = (event: React.DragEvent) => {
    event.preventDefault()
  }

  return (
    <Card>
      <CardHeader>
        <CardTitle>File Upload</CardTitle>
        <CardDescription>
          Upload COMTRADE (.cfg/.dat) or CSV files for playback
        </CardDescription>
      </CardHeader>
      <CardContent className="space-y-4">
        {/* Error Alert */}
        {error && (
          <Alert variant="destructive">
            <AlertCircle className="h-4 w-4" />
            <AlertDescription>{error}</AlertDescription>
          </Alert>
        )}

        {/* File Upload Area */}
        {!file ? (
          <div
            onDrop={handleDrop}
            onDragOver={handleDragOver}
            className="border-2 border-dashed rounded-lg p-8 text-center hover:border-primary transition-colors cursor-pointer"
            onClick={() => fileInputRef.current?.click()}
          >
            <Upload className="mx-auto h-12 w-12 text-muted-foreground mb-4" />
            <p className="text-sm font-medium mb-2">
              {isProcessing ? 'Processing file...' : 'Drop file here or click to browse'}
            </p>
            <p className="text-xs text-muted-foreground">
              Supported formats: .cfg, .dat, .csv
            </p>
            <input
              ref={fileInputRef}
              type="file"
              accept=".cfg,.dat,.csv"
              onChange={handleFileSelect}
              disabled={disabled || isProcessing}
              className="hidden"
            />
          </div>
        ) : (
          <div className="space-y-4">
            {/* File Info */}
            <div className="flex items-center justify-between p-4 border rounded-lg">
              <div className="flex items-center gap-3">
                <FileText className="h-8 w-8 text-primary" />
                <div>
                  <p className="font-medium">{file.name}</p>
                  <p className="text-sm text-muted-foreground">
                    {(file.size / 1024).toFixed(2)} KB
                  </p>
                </div>
              </div>
              <Button
                variant="ghost"
                size="sm"
                onClick={handleClear}
                disabled={disabled}
              >
                <X className="h-4 w-4" />
              </Button>
            </div>

            {/* Metadata Display */}
            {metadata && (
              <div className="grid grid-cols-3 gap-4 p-4 bg-muted rounded-lg">
                <div>
                  <p className="text-xs text-muted-foreground mb-1">Sample Rate</p>
                  <p className="font-medium">{metadata.sampleRate} Hz</p>
                </div>
                <div>
                  <p className="text-xs text-muted-foreground mb-1">Channels</p>
                  <p className="font-medium">{metadata.channels.length}</p>
                </div>
                <div>
                  <p className="text-xs text-muted-foreground mb-1">Total Samples</p>
                  <p className="font-medium">
                    {metadata.totalSamples > 0 ? metadata.totalSamples.toLocaleString() : 'Unknown'}
                  </p>
                </div>
              </div>
            )}

            {/* Channel List */}
            {metadata && metadata.channels.length > 0 && (
              <div>
                <p className="text-sm font-medium mb-2">Channels ({metadata.channels.length})</p>
                <div className="max-h-32 overflow-y-auto border rounded-lg">
                  <table className="w-full text-sm">
                    <thead className="bg-muted">
                      <tr>
                        <th className="text-left p-2">#</th>
                        <th className="text-left p-2">Name</th>
                        <th className="text-left p-2">Type</th>
                        <th className="text-left p-2">Unit</th>
                      </tr>
                    </thead>
                    <tbody>
                      {metadata.channels.map((channel, idx) => (
                        <tr key={idx} className="border-t">
                          <td className="p-2">{idx + 1}</td>
                          <td className="p-2 font-mono text-xs">{channel.name}</td>
                          <td className="p-2">
                            <span className="text-xs px-2 py-1 bg-blue-100 text-blue-700 rounded">
                              {channel.type}
                            </span>
                          </td>
                          <td className="p-2 font-mono text-xs">{channel.unit || '-'}</td>
                        </tr>
                      ))}
                    </tbody>
                  </table>
                </div>
              </div>
            )}
          </div>
        )}
      </CardContent>
    </Card>
  )
}
