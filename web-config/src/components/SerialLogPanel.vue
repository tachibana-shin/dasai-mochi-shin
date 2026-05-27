<script setup lang="ts">
import Button from "primevue/button"
import { useI18n } from "vue-i18n"

defineProps<{
  logs: string[]
  isConnected: boolean
}>()

const emit = defineEmits<{
  clear: []
}>()

const { t } = useI18n()
</script>

<template>
  <footer v-if="isConnected" class="bg-gray-900 rounded-2xl border border-gray-700 overflow-hidden shadow-xl">
    <div class="bg-gray-800 px-4 py-2 border-b border-gray-700 flex justify-between items-center">
      <span class="text-xs font-bold text-gray-400 uppercase tracking-widest flex items-center">
        <div class="w-2 h-2 bg-green-500 rounded-full mr-2 animate-pulse"></div>
        ESP32 Serial Output
      </span>
      <Button icon="pi pi-trash" @click="emit('clear')" text size="small" />
    </div>
    <div class="p-4 h-48 overflow-y-auto font-mono text-[10px] space-y-1 bg-black/50">
      <div v-for="(log, i) in logs" :key="i" class="text-gray-300 border-l-2 border-blue-500/30 pl-2">
        {{ log }}
      </div>
      <div v-if="logs.length === 0" class="text-gray-600 italic">Waiting for logs...</div>
    </div>
  </footer>
</template>
