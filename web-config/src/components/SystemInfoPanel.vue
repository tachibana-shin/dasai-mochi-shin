<script setup lang="ts">
import { filesize } from "filesize"
import { Cpu, Database } from "lucide-vue-next"
import Button from "primevue/button"
import ProgressBar from "primevue/progressbar"
import { computed } from "vue"
import { useI18n } from "vue-i18n"

const props = defineProps<{
  sysInfo: any
  isConnected: boolean
}>()

const emit = defineEmits<{
  refresh: []
}>()

const { t } = useI18n()

function formatSize(kb: number) {
  return filesize(kb * 1024, { base: 2, standard: "jedec" })
}

const ramPercent = computed(() => {
  if (!props.sysInfo) return 0
  return Math.round(((props.sysInfo.ram_total - props.sysInfo.ram_free) / props.sysInfo.ram_total) * 100)
})

const flashPercent = computed(() => {
  if (!props.sysInfo) return 0
  return Math.round(((props.sysInfo.flash_total - props.sysInfo.flash_free) / props.sysInfo.flash_total) * 100)
})

const sdPercent = computed(() => {
  if (!props.sysInfo || !props.sysInfo.sd_ok) return 0
  return Math.round(((props.sysInfo.sd_total - props.sysInfo.sd_free) / props.sysInfo.sd_total) * 100)
})
</script>

<template>
  <section v-if="isConnected && sysInfo" class="bg-gray-800 p-6 rounded-2xl border border-gray-700 space-y-6 shadow-xl">
    <div class="flex items-center justify-between">
      <h2 class="font-bold text-gray-300 flex items-center"><Cpu class="w-4 h-4 mr-2" /> {{ t("system.title") }}</h2>
      <Button icon="pi pi-refresh" text rounded @click="emit('refresh')" class="w-8 h-8" />
    </div>

    <div class="space-y-4">
      <div class="p-4 bg-gray-900/50 rounded-xl border border-gray-700/50">
        <div class="flex justify-between text-xs text-gray-400 mb-1">
          <span>{{ t("system.ram") }}</span>
          <span>{{ formatSize(sysInfo.ram_total - sysInfo.ram_free) }} / {{ formatSize(sysInfo.ram_total) }}</span>
        </div>
        <ProgressBar :value="ramPercent" class="h-2" />
      </div>

      <div class="p-4 bg-gray-900/50 rounded-xl border border-gray-700/50">
        <div class="flex justify-between text-xs text-gray-400 mb-1">
          <span>{{ t("system.flash") }}</span>
          <span
            >{{ formatSize(sysInfo.flash_total - sysInfo.flash_free) }} / {{ formatSize(sysInfo.flash_total) }}</span
          >
        </div>
        <ProgressBar :value="flashPercent" color="#10b981" class="h-2" />
      </div>

      <div class="p-4 bg-gray-900/50 rounded-xl border border-gray-700/50">
        <div class="flex justify-between text-xs text-gray-400 mb-1">
          <span>{{ t("system.sdcard") }}</span>
          <span v-if="sysInfo.sd_ok"
            >{{ formatSize(sysInfo.sd_total - sysInfo.sd_free) }} / {{ formatSize(sysInfo.sd_total) }}</span
          >
          <span v-else class="text-red-500 font-bold italic">{{ t("system.sd_not_found") }}</span>
        </div>
        <ProgressBar v-if="sysInfo.sd_ok" :value="sdPercent" color="#8b5cf6" class="h-2" />
        <div v-else class="h-2 bg-gray-700 rounded-full opacity-30"></div>
      </div>

      <div class="p-4 bg-blue-500/10 border border-blue-500/30 rounded-xl flex items-start space-x-3">
        <Database class="w-4 h-4 text-blue-400 mt-0.5" />
        <p class="text-[10px] text-blue-200 leading-relaxed">{{ t("system.storage_notice") }}</p>
      </div>

      <div class="grid grid-cols-1 gap-2 text-sm">
        <div class="flex justify-between py-2 border-b border-gray-700/50">
          <span class="text-gray-400">IP</span>
          <span class="font-mono text-xs text-blue-400" v-if="sysInfo.ip">{{ sysInfo.ip }}</span>
          <span class="font-mono text-xs text-gray-600" v-else>—</span>
        </div>
        <div class="flex justify-between py-2 border-b border-gray-700/50">
          <span class="text-gray-400">{{ t("system.cpu") }}</span>
          <span class="font-mono text-xs">{{ sysInfo.cpu }}</span>
        </div>
        <div class="flex justify-between py-2 border-b border-gray-700/50">
          <span class="text-gray-400">Firmware</span>
          <span class="text-xs">v1.2.0-stable</span>
        </div>
      </div>
    </div>
  </section>

  <section
    v-else
    class="bg-gray-800 p-8 rounded-2xl border border-gray-700 flex flex-col items-center justify-center text-center space-y-4 h-64 shadow-xl"
  >
    <Database class="w-12 h-12 text-gray-700" />
    <p class="text-sm text-gray-500">{{ isConnected ? "Loading..." : t("common.disconnected") }}</p>
  </section>
</template>
