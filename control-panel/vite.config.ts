import {defineConfig} from 'vite'
import react from '@vitejs/plugin-react-swc'
import tailwindcss from '@tailwindcss/vite'
import {VitePWA} from 'vite-plugin-pwa'

export default defineConfig(({command}) => ({
  base: command === 'build' ? '/tennis-gun/' : '/',
  plugins: [
    react(),
    tailwindcss(),
    VitePWA({
      registerType: 'autoUpdate',
      includeAssets: ['tennis-gun.svg'],
      manifest: {
        name: 'Tennis Gun Control',
        short_name: 'Tennis Gun',
        description: 'Bluetooth control panel for Tennis Gun',
        theme_color: '#ffffff',
        background_color: '#ffffff',
        display: 'standalone',
        start_url: '/tennis-gun/',
        scope: '/tennis-gun/',
        icons: [{
          src: 'tennis-gun.svg',
          sizes: 'any',
          type: 'image/svg+xml',
          purpose: 'any maskable',
        }],
      },
      workbox: {
        navigateFallback: '/tennis-gun/index.html',
        globPatterns: ['**/*.{html,js,css,svg,webmanifest}'],
      },
    }),
  ],
  build: {
    outDir: 'dist',
    emptyOutDir: true,
  },
}))
