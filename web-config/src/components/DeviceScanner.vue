<script setup lang="ts">
import { Monitor } from "lucide-vue-next"
import Button from "primevue/button"
import { useI18n } from "vue-i18n"

defineProps<{
  isScanning: boolean
  devices: { name: string; ip: string; url: string }[]
}>()

const emit = defineEmits<{
  connect: [url: string]
  scan: []
}>()

const { t } = useI18n()
</script>

<template>
  <div class="flex-grow flex flex-col items-center justify-center space-y-6 p-12">
    <Monitor class="w-20 h-20 text-gray-700" />
    <div class="text-center">
      <p class="text-gray-400 font-medium">{{ t("common.disconnected") }}</p>
      <p class="text-xs text-gray-500 mt-1">Select a device or enter host address</p>
    </div>

    <div v-if="devices.length > 0" class="w-full max-w-sm space-y-2">
      <div
        v-for="device in devices"
        :key="device.url"
        @click="emit('connect', device.url)"
        class="p-4 bg-gray-900/50 rounded-xl border border-gray-700 hover:border-blue-500 cursor-pointer flex items-center justify-between group"
      >
        <div>
          <p class="font-bold text-sm">{{ device.name }}</p>
          <p class="text-[10px] text-gray-500">{{ device.url }}</p>
        </div>
        <i class="pi pi-chevron-right text-gray-600 group-hover:text-blue-500"></i>
      </div>
    </div>

    <Button
      v-if="devices.length === 0"
      :label="isScanning ? 'Scanning...' : 'Scan for Devices'"
      icon="pi pi-search"
      @click="emit('scan')"
      :loading="isScanning"
      class="p-button-lg px-8 rounded-xl"
    />
  </div>
</template>
