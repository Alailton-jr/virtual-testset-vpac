import { Card, CardContent, CardDescription, CardHeader, CardTitle } from '@/components/ui/card'

export default function ComtradePage() {
  return (
    <div className="space-y-6">
      <div>
        <h1 className="text-3xl font-bold tracking-tight">COMTRADE Playback</h1>
        <p className="text-muted-foreground">
          Upload and play COMTRADE files through SV streams
        </p>
      </div>

      <Card>
        <CardHeader>
          <CardTitle>File Upload</CardTitle>
          <CardDescription>
            Upload COMTRADE files (.cfg/.dat) and map channels to SV streams
          </CardDescription>
        </CardHeader>
        <CardContent>
          <p className="text-sm text-muted-foreground">
            COMTRADE upload and playback interface coming soon...
          </p>
        </CardContent>
      </Card>
    </div>
  )
}
