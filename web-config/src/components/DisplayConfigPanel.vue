<script setup lang="ts">
import InputNumber from "primevue/inputnumber"
import Slider from "primevue/slider"
import ToggleSwitch from "primevue/toggleswitch"
import { useI18n } from "vue-i18n"

import type { AppConfig } from "../types/config"

defineProps<{
  config: AppConfig
}>()

const { t } = useI18n()
</script>

<template>
  <div v-if="config" class="space-y-8">
    <div class="flex justify-between items-center p-6 bg-gray-900/30 rounded-2xl border border-gray-700">
      <div class="space-y-1">
        <h3 class="font-bold">{{ t("display.brightness") }}</h3>
        <p class="text-xs text-gray-500">OLED Brightness Control</p>
      </div>
      <div class="w-64 flex items-center space-x-4">
        <Slider v-model="config.brightness" :min="0" :max="255" class="flex-grow" />
        <InputNumber v-model="config.brightness" :min="0" :max="255" class="w-20" size="small" />
      </div>
    </div>

    <div class="grid grid-cols-2 gap-4">
      <div class="flex flex-col space-y-2">
        <label class="text-xs font-bold text-gray-500">Auto On (Hour)</label>
        <InputNumber v-model="config.autoOnHour" :min="-1" :max="23" showButtons />
      </div>
      <div class="flex flex-col space-y-2">
        <label class="text-xs font-bold text-gray-500">Auto Off (Hour)</label>
        <InputNumber v-model="config.autoOffHour" :min="-1" :max="23" showButtons />
      </div>
    </div>

    <div class="pt-6 border-t border-gray-700 space-y-4">
      <h3 class="text-lg font-bold">Advanced Screen Settings</h3>
      <div class="flex items-center justify-between p-4 bg-gray-900/30 rounded-lg">
        <label class="font-medium text-sm">Flip Screen (180°)</label>
        <ToggleSwitch v-model="config.screenFlipMode" />
      </div>
      <div class="flex items-center justify-between p-4 bg-gray-900/30 rounded-lg">
        <label class="font-medium text-sm">Invert Screen Colors</label>
        <ToggleSwitch v-model="config.screenNegative" />
      </div>
      <div class="grid grid-cols-2 gap-6">
        <div class="flex flex-col space-y-2">
          <label class="text-xs font-bold text-gray-500">Screen Width (px)</label>
          <InputNumber v-model="config.screenWidth" />
        </div>
        <div class="flex flex-col space-y-2">
          <label class="text-xs font-bold text-gray-500">Screen Height (px)</label>
          <InputNumber v-model="config.screenHeight" />
        </div>
      </div>
    </div>
  </div>
</template>
