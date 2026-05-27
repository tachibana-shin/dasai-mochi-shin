import { defineConfig } from "oxlint"

export default defineConfig({
  plugins: ["import", "typescript"],
  categories: {
    correctness: "warn",
    suspicious: "warn",
    pedantic: "deny",
  },
  rules: {
    "no-debugger": "error",
    "max-lines-per-function": "off",
    "max-dependencies": "off",
    "max-depth": "off"
  },
})
