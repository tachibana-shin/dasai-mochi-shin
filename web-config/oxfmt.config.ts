import { defineConfig } from "oxfmt"

export default defineConfig({
  sortImports: {
    groups: [
      "type-import",
      ["value-builtin", "value-external"],
      "type-internal",
      "value-internal",
      ["type-parent", "type-sibling", "type-index"],
      ["value-parent", "value-sibling", "value-index"],
      "unknown"
    ]
  },
  semi: false,
  singleQuote: false,
  tabWidth: 2,
  trailingComma: "all",
  printWidth: 120,
  endOfLine: "lf"
})
