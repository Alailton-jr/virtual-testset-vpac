import { defineConfig, loadEnv } from 'vite'
import react from '@vitejs/plugin-react'
import path from 'path'

// https://vite.dev/config/
export default defineConfig(({ mode }) => {
  // Load env file based on `mode` in the current working directory.
  // Set the third parameter to '' to load all env regardless of the `VITE_` prefix.
  const env = loadEnv(mode, process.cwd(), '')
  
  // Get backend URLs from environment variables or use defaults
  const apiUrl = env.VITE_API_URL || 'http://localhost:8081'
  const wsUrl = env.VITE_WS_URL || 'ws://localhost:8082'

  return {
    plugins: [react()],
    resolve: {
      alias: {
        '@': path.resolve(__dirname, './src'),
      },
    },
    server: {
      proxy: {
        // Proxy API requests to backend HTTP server
        '/api': {
          target: apiUrl,
          changeOrigin: true,
        },
        // Proxy WebSocket connections
        '/ws': {
          target: wsUrl,
          ws: true,
          changeOrigin: true,
        },
      },
    },
  }
})
