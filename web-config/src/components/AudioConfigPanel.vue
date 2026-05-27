<script setup lang="ts">
import Button from "primevue/button"
import InputText from "primevue/inputtext"
import { useI18n } from "vue-i18n"

import type { AppConfig } from "../types/config"

defineProps<{
  config: AppConfig
}>()

const emit = defineEmits<{
  preview: [path: string]
}>()

const { t } = useI18n()
</script>

<template>
  <div class="space-y-8">
    <div class="flex items-center justify-between">
      <h2 class="text-xl font-bold">Audio Settings</h2>
      <div class="flex space-x-2">
        <Button :label="'Upload Audio'" icon="pi pi-upload" severity="success" size="small" />
      </div>
    </div>
    <div v-if="config" class="grid grid-cols-1 gap-6">
      <div class="flex flex-col space-y-2">
        <label class="text-xs font-bold text-gray-500">Click Sound Path</label>
        <div class="flex space-x-2">
          <InputText v-model="config.customClickSoundPath" class="flex-grow" />
          <Button
            label="Preview"
            icon="pi pi-play"
            text
            size="small"
            @click="emit('preview', config.customClickSoundPath)"
          />
        </div>
      </div>
      <div class="flex flex-col space-y-2">
        <label class="text-xs font-bold text-gray-500">Notify Sound Path</label>
        <div class="flex space-x-2">
          <InputText v-model="config.customNotifySoundPath" class="flex-grow" />
          <Button
            label="Preview"
            icon="pi pi-play"
            text
            size="small"
            @click="emit('preview', config.customNotifySoundPath)"
          />
        </div>
      </div>
    </div>
  </div>
</template>
