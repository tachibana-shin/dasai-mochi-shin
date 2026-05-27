import { defineConfig } from "vite"
import vue from "@vitejs/plugin-vue"
import tailwindcss from "@tailwindcss/vite"
import compression from "vite-plugin-compression"

export default defineConfig({
  plugins: [
    vue(),
    tailwindcss(),
    compression({ algorithm: "gzip", ext: ".gz", deleteOriginFile: true }),
  ],
  server: {
    port: 3000,
    proxy: {
      "/api": "http://192.168.0.42/api"
    }
  },
  build: {
    target: "esnext"
    // Rolldown will be the default in future Vite, current Vite 6+ supports it better
  }
})
