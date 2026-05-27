<script setup lang="ts">
import { ref } from "vue"
import { useI18n } from "vue-i18n"
import InputNumber from "primevue/inputnumber"
import ToggleSwitch from "primevue/toggleswitch"
import Button from "primevue/button"
import { Smile } from "lucide-vue-next"
import type { AppConfig } from "../types/config"

const props = defineProps<{
  config: AppConfig
  mochiFiles: { name: string; size: number }[]
  isUploading: boolean
  formatSize: (kb: number) => string
}>()

const emit = defineEmits<{
  refresh: []
  upload: []
  delete: [name: string]
  fileSelected: [event: Event]
}>()

const { t } = useI18n()
const fileInput = ref<HTMLInputElement | null>(null)

function triggerUpload() {
  fileInput.value?.click()
}

defineExpose({ fileInput })
</script>

<template>
  <div class="space-y-8">
    <div class="flex items-center justify-between">
      <h2 class="text-xl font-bold">{{ t('mochi.manage') }}</h2>
      <div class="flex space-x-2">
        <Button icon="pi pi-refresh" @click="emit('refresh')" text />
        <input type="file" ref="fileInput" class="hidden" @change="emit('fileSelected', $event)" accept=".qgif,.qgif+" />
        <Button :label="isUploading ? 'Uploading...' : t('mochi.upload')" icon="pi pi-upload" severity="success" size="small" @click="triggerUpload" :loading="isUploading" />
      </div>
    </div>

    <div class="grid grid-cols-2 md:grid-cols-3 gap-4">
      <div v-for="file in mochiFiles" :key="file.name" class="p-4 bg-gray-900/50 rounded-xl border border-gray-700 flex flex-col items-center space-y-3 group hover:border-blue-500/50 transition-all relative">
        <div class="w-16 h-16 bg-gray-800 rounded-lg flex items-center justify-center text-gray-600">
          <Smile v-if="file.name.includes('smile')" class="text-blue-400" />
          <Smile v-else />
        </div>
        <span class="text-xs font-mono truncate w-full text-center">{{ file.name }}</span>
        <span class="text-[10px] text-gray-500">{{ formatSize(file.size / 1024) }}</span>

        <div class="absolute top-2 right-2 flex space-x-1 opacity-0 group-hover:opacity-100 transition-opacity">
          <Button icon="pi pi-pencil" severity="info" text rounded size="small" />
          <Button icon="pi pi-eye" severity="help" text rounded size="small" />
          <Button icon="pi pi-trash" severity="danger" text rounded size="small" @click="emit('delete', file.name)" />
        </div>
      </div>
    </div>

    <div v-if="config" class="pt-8 border-t border-gray-700 space-y-6">
      <h2 class="text-lg font-bold">{{ t('mochi.animation') }}</h2>
      <div class="grid grid-cols-2 gap-6">
        <div class="flex flex-col space-y-2">
          <label class="text-xs font-bold text-gray-500">{{ t('mochi.animation') }} Divisor</label>
          <InputNumber v-model="config.mochiSpeedDivisor" :min="1" :max="10" showButtons />
        </div>
        <div class="flex flex-col space-y-2">
          <label class="text-xs font-bold text-gray-500">{{ t('mochi.interval') }} (ms)</label>
          <InputNumber v-model="config.mochiClockInterval" :min="0" :step="1000" showButtons />
        </div>
        <div class="flex flex-col space-y-2">
          <label class="text-xs font-bold text-gray-500">Animation Duration (ms)</label>
          <InputNumber v-model="config.mochiClockDuration" :min="0" :step="500" showButtons />
        </div>
      </div>
      <div class="flex items-center justify-between p-4 bg-gray-900/30 rounded-lg">
        <label class="font-medium text-sm">Invert Mochi Colors</label>
        <ToggleSwitch v-model="config.mochiNegative" />
      </div>
    </div>
  </div>
</template>
