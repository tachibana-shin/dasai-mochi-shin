<script setup lang="ts">
import { Monitor, RefreshCw, Smile, Wifi } from "lucide-vue-next"
import Tab from "primevue/tab"
import TabList from "primevue/tablist"
import TabPanel from "primevue/tabpanel"
import TabPanels from "primevue/tabpanels"
import Tabs from "primevue/tabs"
import { onMounted, ref, watch } from "vue"
import { useI18n } from "vue-i18n"

import type { AppConfig } from "./types/config"

import AudioConfigPanel from "./components/AudioConfigPanel.vue"
import ConfigActionBar from "./components/ConfigActionBar.vue"
import DashboardHeader from "./components/DashboardHeader.vue"
import DeviceScanner from "./components/DeviceScanner.vue"
import DisplayConfigPanel from "./components/DisplayConfigPanel.vue"
import MochiConfigPanel from "./components/MochiConfigPanel.vue"
import SerialLogPanel from "./components/SerialLogPanel.vue"
import SystemConfigPanel from "./components/SystemConfigPanel.vue"
import SystemInfoPanel from "./components/SystemInfoPanel.vue"
import WiFiConfigPanel from "./components/WiFiConfigPanel.vue"
import { useWiFi } from "./composables/useWiFi"

const { t } = useI18n()
const { isConnected, baseUrl, devices, isScanning, connect, disconnect, autoConnect, scan, request, uploadFile } =
  useWiFi()
const config = ref<AppConfig | null>(null)
const sysInfo = ref<any>(null)
const mochiFiles = ref<{ name: string; size: number }[]>([])
const logs = ref<string[]>([])
const isUploading = ref(false)
const isSaving = ref(false)

const fetchConfig = async () => {
  config.value = await request("/api/config")
}
const fetchSysInfo = async () => {
  sysInfo.value = await request("/api/info")
}
const fetchMochiFiles = async () => {
  mochiFiles.value = await request("/api/files/list")
}

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

const previewAudio = async (path: string) => {
  await fetch(`${baseUrl.value}/api/preview/audio`, {
    method: "POST",
    headers: { "Content-Type": "application/json" },
    body: JSON.stringify({ path }),
  })
}

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

onMounted(async () => {
  await autoConnect()
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

      <div class="col-span-12 lg:col-span-8 bg-gray-800 rounded-2xl border border-gray-700 shadow-xl flex flex-col">
        <DeviceScanner
          v-if="!isConnected"
          :isScanning="isScanning"
          :devices="devices"
          @connect="connect($event)"
          @scan="scan"
        />

        <Tabs v-else value="0" class="flex-grow flex flex-col">
          <TabList class="px-6 pt-2 bg-gray-800/50 border-b border-gray-700 overflow-x-auto flex-shrink-0">
            <Tab value="0"><Wifi class="w-4 h-4 mr-2" /> {{ t("tabs.wifi") }}</Tab>
            <Tab value="1"><Monitor class="w-4 h-4 mr-2" /> {{ t("tabs.display") }}</Tab>
            <Tab value="2"><Smile class="w-4 h-4 mr-2" /> {{ t("tabs.mochi") }}</Tab>
            <Tab value="3"><i class="pi pi-volume-up mr-2" /> {{ t("tabs.audio") }}</Tab>
            <Tab value="4"><RefreshCw class="w-4 h-4 mr-2" /> {{ t("tabs.system") }}</Tab>
          </TabList>

          <TabPanels class="flex-grow p-8 overflow-y-auto bg-gray-800">
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
                @refresh="fetchMochiFiles"
                @delete="deleteFile"
                @fileSelected="handleFileUpload"
              />
            </TabPanel>
            <TabPanel value="3">
              <AudioConfigPanel :config="config!" @preview="previewAudio" />
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
