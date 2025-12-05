import {defineConfig} from 'vite'
import react from '@vitejs/plugin-react-swc'
import tailwindcss from '@tailwindcss/vite'
import viteCompression from 'vite-plugin-compression';

// https://vite.dev/config/
export default defineConfig({
    plugins: [react(), tailwindcss(), viteCompression({
        algorithm: 'gzip',
        deleteOriginFile: true
    })],
    build: {
        outDir: "../control-board/data",
        emptyOutDir: true,
    }
})
