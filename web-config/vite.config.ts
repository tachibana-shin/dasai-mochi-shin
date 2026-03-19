import { defineConfig } from "vite"
import vue from "@vitejs/plugin-vue"
import tailwindcss from "@tailwindcss/vite"

export default defineConfig({
  plugins: [vue(), tailwindcss()],
  server: {
    port: 3000
  },
  build: {
    target: "esnext"
    // Rolldown will be the default in future Vite, current Vite 6+ supports it better
  }
})
