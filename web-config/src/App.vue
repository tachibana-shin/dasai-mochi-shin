<script setup lang="ts">
import { ref, onMounted, watch } from "vue"
import { useWiFi } from "./composables/useWiFi"
import type { AppConfig } from "./types/config"
import { useI18n } from "vue-i18n"
import Tabs from "primevue/tabs"
import TabList from "primevue/tablist"
import Tab from "primevue/tab"
import TabPanels from "primevue/tabpanels"
import TabPanel from "primevue/tabpanel"
import { Wifi, Monitor, Smile, RefreshCw } from "lucide-vue-next"

import DashboardHeader from "./components/DashboardHeader.vue"
import DeviceScanner from "./components/DeviceScanner.vue"
import SystemInfoPanel from "./components/SystemInfoPanel.vue"
import WiFiConfigPanel from "./components/WiFiConfigPanel.vue"
import DisplayConfigPanel from "./components/DisplayConfigPanel.vue"
import MochiConfigPanel from "./components/MochiConfigPanel.vue"
import AudioConfigPanel from "./components/AudioConfigPanel.vue"
import SystemConfigPanel from "./components/SystemConfigPanel.vue"
import ConfigActionBar from "./components/ConfigActionBar.vue"
import SerialLogPanel from "./components/SerialLogPanel.vue"

const { t } = useI18n()
const { isConnected, baseUrl, devices, isScanning, connect, disconnect, scan, request, uploadFile } = useWiFi()
const config = ref<AppConfig | null>(null)
const sysInfo = ref<any>(null)
const mochiFiles = ref<{ name: string, size: number }[]>([])
const logs = ref<string[]>([])
const isUploading = ref(false)
const isSaving = ref(false)

const fetchConfig = async () => { config.value = await request("/api/config") }
const fetchSysInfo = async () => { sysInfo.value = await request("/api/info") }
const fetchMochiFiles = async () => { mochiFiles.value = await request("/api/files/list") }

watch(isConnected, (connected) => {
  if (connected) {
    setTimeout(async () => {
      await fetchConfig()
      await fetchSysInfo()
      await fetchMochiFiles()
    }, 500)
  } else {
    config.value = null
    sysInfo.value = null
    mochiFiles.value = []
    logs.value = []
  }
})

const saveConfig = async () => {
  isSaving.value = true
  await request("/api/config", "POST", config.value)
  isSaving.value = false
}
const reboot = async () => await request("/api/reboot", "POST")

const deleteFile = async (name: string) => {
  if (confirm(t("mochi.delete") + " " + name + "?")) {
    await request(`/api/files/delete?path=${config.value?.homePath}/Mochi/${name}`, "DELETE")
    setTimeout(fetchMochiFiles, 500)
  }
}

const handleFileUpload = async (event: Event) => {
  const target = event.target as HTMLInputElement
  if (!target.files || target.files.length === 0) return
  const file = target.files[0]
  isUploading.value = true
  try {
    await uploadFile(file)
    setTimeout(fetchMochiFiles, 1000)
  } catch (error) {
    console.error("Upload failed", error)
    alert("Upload failed: " + error)
  } finally {
    isUploading.value = false
  }
}

const formatSize = (kb: number): string => {
  if (!kb || kb === 0) return '0 B'
  const sizes = ['B', 'KB', 'MB', 'GB']
  const i = Math.floor(Math.log(kb) / Math.log(1024))
  return parseFloat((kb / Math.pow(1024, i)).toFixed(1)) + ' ' + sizes[i]
}

onMounted(() => {
  if (isConnected.value) {
    fetchConfig()
    fetchSysInfo()
    fetchMochiFiles()
  }
})
</script>

<template>
  <div class="min-h-screen flex flex-col p-6 max-w-5xl mx-auto space-y-6">
    <DashboardHeader
      :isConnected="isConnected"
      :baseUrl="baseUrl"
      :isScanning="isScanning"
      :sysInfo="sysInfo"
      @update:baseUrl="baseUrl = $event"
      @connect="connect()"
      @disconnect="disconnect"
      @scan="scan"
    />

    <main class="flex-grow grid grid-cols-12 gap-6">
      <aside class="col-span-12 lg:col-span-4 space-y-6">
        <SystemInfoPanel :sysInfo="sysInfo" :isConnected="isConnected" @refresh="fetchSysInfo" />
      </aside>

      <div class="col-span-12 lg:col-span-8 bg-gray-800 rounded-2xl border border-gray-700 shadow-xl overflow-hidden flex flex-col">
        <DeviceScanner
          v-if="!isConnected"
          :isScanning="isScanning"
          :devices="devices"
          @connect="connect($event)"
          @scan="scan"
        />

        <Tabs v-else value="0" class="flex-grow flex flex-col">
          <TabList class="px-6 pt-2 bg-gray-800/50 border-b border-gray-700">
            <Tab value="0"><Wifi class="w-4 h-4 mr-2" /> {{ t('tabs.wifi') }}</Tab>
            <Tab value="1"><Monitor class="w-4 h-4 mr-2" /> {{ t('tabs.display') }}</Tab>
            <Tab value="2"><Smile class="w-4 h-4 mr-2" /> {{ t('tabs.mochi') }}</Tab>
            <Tab value="3"><i class="pi pi-volume-up mr-2" /> {{ t('tabs.audio') }}</Tab>
            <Tab value="4"><RefreshCw class="w-4 h-4 mr-2" /> {{ t('tabs.system') }}</Tab>
          </TabList>

          <TabPanels class="flex-grow p-8 overflow-y-auto">
            <TabPanel value="0">
              <WiFiConfigPanel :config="config!" @refresh="fetchConfig" />
            </TabPanel>
            <TabPanel value="1">
              <DisplayConfigPanel :config="config!" />
            </TabPanel>
            <TabPanel value="2">
              <MochiConfigPanel
                :config="config!"
                :mochiFiles="mochiFiles"
                :isUploading="isUploading"
                :formatSize="formatSize"
                @refresh="fetchMochiFiles"
                @delete="deleteFile"
                @fileSelected="handleFileUpload"
              />
            </TabPanel>
            <TabPanel value="3">
              <AudioConfigPanel :config="config!" />
            </TabPanel>
            <TabPanel value="4">
              <SystemConfigPanel :config="config!" />
            </TabPanel>
          </TabPanels>

          <ConfigActionBar :isSaving="isSaving" @save="saveConfig" @reboot="reboot" />
        </Tabs>
      </div>
    </main>

    <SerialLogPanel :logs="logs" :isConnected="isConnected" @clear="logs = []" />
  </div>
</template>

<style>
@import "tailwindcss";
@import "primeicons/primeicons.css";

:root {
  --primary-color: #3b82f6;
}

body {
  background-color: #0b0f1a;
  color: #e2e8f0;
  font-family: 'Inter', system-ui, sans-serif;
}

.p-tabs { background: transparent !important; }
.p-tablist { background: transparent !important; }
.p-tablist-tab {
  background: transparent !important;
  color: #64748b !important;
  font-weight: 600 !important;
  font-size: 0.875rem !important;
}
.p-tablist-tab:hover { color: #94a3b8 !important; }
.p-tablist-active-bar { background: var(--primary-color) !important; height: 3px !important; }
.p-tablist-tab[data-p-active="true"] { color: white !important; }

.p-inputtext, .p-inputnumber-input {
  background: #111827 !important;
  border: 1px solid #1f2937 !important;
  color: white !important;
  border-radius: 0.75rem !important;
}

.p-inputtext:focus { border-color: var(--primary-color) !important; box-shadow: 0 0 0 2px rgba(59, 130, 246, 0.2) !important; }

.p-progressbar { background: #1f2937 !important; border-radius: 1rem !important; overflow: hidden; }
.p-progressbar-value { background: var(--primary-color) !important; transition: width 0.5s ease; }

.p-button { border-radius: 0.75rem !important; transition: all 0.2s; }
.p-button:hover { transform: translateY(-1px); }

.p-selectbutton .p-button { font-size: 0.7rem !important; padding: 0.25rem 0.75rem !important; }
</style>
