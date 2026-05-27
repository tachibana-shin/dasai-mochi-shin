<script setup lang="ts">
import { useI18n } from "vue-i18n"
import InputText from "primevue/inputtext"
import Button from "primevue/button"
import { Wifi } from "lucide-vue-next"
import type { AppConfig } from "../types/config"

defineProps<{
  config: AppConfig
}>()

const emit = defineEmits<{
  refresh: []
}>()

const { t } = useI18n()
</script>

<template>
  <div v-if="config" class="space-y-6">
    <div class="flex items-center justify-between mb-4">
      <div>
        <h2 class="text-xl font-bold">{{ t('tabs.wifi') }}</h2>
        <p class="text-xs text-gray-500">Manage saved networks (Priority from top to bottom)</p>
      </div>
      <div class="flex space-x-2">
        <Button icon="pi pi-plus" :label="t('common.add')" @click="config.wifi.push({ssid: '', pass: ''})" severity="success" size="small" />
        <Button icon="pi pi-sync" @click="emit('refresh')" text size="small" />
      </div>
    </div>

    <div class="space-y-4">
      <div v-for="(net, index) in config.wifi" :key="index"
           class="p-4 bg-gray-900/40 rounded-2xl border border-gray-700/50 flex flex-col md:flex-row gap-4 items-end relative group">
        <div class="flex-grow grid grid-cols-1 md:grid-cols-2 gap-4 w-full">
          <div class="flex flex-col space-y-2">
            <label class="text-[10px] uppercase font-bold text-gray-500 ml-1">SSID</label>
            <InputText v-model="net.ssid" placeholder="Network Name" class="w-full" />
          </div>
          <div class="flex flex-col space-y-2">
            <label class="text-[10px] uppercase font-bold text-gray-500 ml-1">Password</label>
            <InputText v-model="net.pass" type="password" placeholder="••••••••" class="w-full" />
          </div>
        </div>
        <Button icon="pi pi-trash" severity="danger" text @click="config.wifi.splice(index, 1)"
                class="mb-0.5 hover:bg-red-500/10" v-if="config.wifi.length > 1" />
        <div class="absolute -left-2 top-1/2 -translate-y-1/2 w-1 h-8 bg-blue-500/20 rounded-full group-hover:bg-blue-500 transition-colors"></div>
      </div>

      <div v-if="config.wifi.length === 0" class="text-center py-8 bg-gray-900/20 rounded-2xl border-2 border-dashed border-gray-800">
        <Wifi class="w-8 h-8 text-gray-700 mx-auto mb-2" />
        <p class="text-gray-500 text-sm">No networks saved. Click "Add" to start.</p>
      </div>
    </div>
  </div>
</template>
