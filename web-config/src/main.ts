import { createApp } from "vue"
import { createI18n } from "vue-i18n"
import App from "./App.vue"
import PrimeVue from "primevue/config"
import Aura from "@primevue/themes/aura"
import en from "./locales/en"
import vi from "./locales/vi"
import "./style.css"

const i18n = createI18n({
  legacy: false,
  locale: "vi",
  messages: { en, vi }
})

const app = createApp(App)
app.use(i18n)
app.use(PrimeVue, {
  theme: {
    preset: Aura,
    options: {
      darkModeSelector: "system"
    }
  }
})
app.mount("#app")
