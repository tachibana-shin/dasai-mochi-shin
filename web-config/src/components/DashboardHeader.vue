<script setup lang="ts">
import { useI18n } from "vue-i18n"
import InputText from "primevue/inputtext"
import SelectButton from "primevue/selectbutton"
import Button from "primevue/button"
import { Smile, Globe } from "lucide-vue-next"

defineProps<{
  isConnected: boolean
  baseUrl: string
  isScanning: boolean
  sysInfo: any
}>()

const emit = defineEmits<{
  "update:baseUrl": [value: string]
  "update:locale": [value: string]
  connect: []
  disconnect: []
  scan: []
}>()

const { t, locale } = useI18n()
</script>

<template>
  <header class="flex justify-between items-center bg-gray-800 p-6 rounded-2xl shadow-xl border border-gray-700">
    <div class="flex items-center space-x-3">
      <div class="w-12 h-12 bg-blue-600 rounded-xl flex items-center justify-center shadow-lg shadow-blue-500/20">
        <Smile class="text-white w-7 h-7" />
      </div>
      <div>
        <h1 class="text-xl font-black tracking-tight">MOCHI DASHBOARD</h1>
        <div class="flex items-center text-[10px] space-x-2">
          <span class="text-gray-400 uppercase font-bold">{{ sysInfo?.module || "Dasai Mochi Shin" }}</span>
          <span class="w-1 h-1 bg-gray-600 rounded-full"></span>
          <span class="text-blue-400 font-mono">v1.2.0</span>
        </div>
      </div>
    </div>

    <div class="flex items-center space-x-4">
      <div class="bg-gray-900/50 p-1 rounded-lg border border-gray-700 flex items-center">
        <Globe class="w-4 h-4 mx-2 text-gray-500" />
        <SelectButton v-model="locale" :options="['vi', 'en']" aria-labelledby="basic" class="text-xs" />
      </div>

      <div v-if="!isConnected" class="flex items-center space-x-2">
        <InputText :modelValue="baseUrl" @update:modelValue="emit('update:baseUrl', $event)" placeholder="http://mochishin.local" class="w-48 text-xs" />
        <Button :label="t('common.connect')" icon="pi pi-link" @click="emit('connect')" :loading="isScanning" class="rounded-full font-bold" />
        <Button icon="pi pi-search" @click="emit('scan')" :loading="isScanning" text rounded />
      </div>
      <Button v-else :label="t('common.disconnect')" severity="danger" outlined @click="emit('disconnect')" class="rounded-full font-bold" />
    </div>
  </header>
</template>
