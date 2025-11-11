import { Sidebar } from '@/components/Sidebar'
import { Topbar } from '@/components/Topbar'

interface AppLayoutProps {
  children: React.ReactNode
}

export function AppLayout({ children }: AppLayoutProps) {
  return (
    <div className="flex h-screen overflow-hidden">
      {/* Sidebar */}
      <aside className="hidden w-64 flex-col md:flex">
        <Sidebar />
      </aside>

      {/* Main content */}
      <div className="flex flex-1 flex-col overflow-hidden">
        <Topbar />
        <main className="flex-1 overflow-y-auto p-6">
          <div className="container max-w-screen-2xl">
            {children}
          </div>
        </main>
      </div>
    </div>
  )
}
