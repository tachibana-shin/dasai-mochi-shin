<script setup lang="ts">
import { ref, onMounted, onUnmounted, computed, watch } from "vue"
import { useSerial } from "./composables/useSerial"
import type { AppConfig } from "./types/config"
import { useI18n } from "vue-i18n"
import Tabs from "primevue/tabs"
import TabList from "primevue/tablist"
import Tab from "primevue/tab"
import TabPanels from "primevue/tabpanels"
import TabPanel from "primevue/tabpanel"
import Button from "primevue/button"
import InputText from "primevue/inputtext"
import InputNumber from "primevue/inputnumber"
import Slider from "primevue/slider"
import ToggleSwitch from "primevue/toggleswitch"
import SelectButton from "primevue/selectbutton"
import ProgressBar from "primevue/progressbar"
import {
  Monitor,
  Wifi,
  Droplets,
  Smile,
  Save,
  RefreshCw,
  Cpu,
  Database,
  Trash2,
  Upload,
  Globe
} from "lucide-vue-next"
import { filesize } from "filesize"

const { t, locale } = useI18n()
const { isConnected, connect, disconnect, send } = useSerial()
const config = ref<AppConfig | null>(null)
const sysInfo = ref<any>(null)
const mochiFiles = ref<string[]>([])
const logs = ref<string[]>([])

const fetchConfig = async () => await send("GET_CONFIG")
const fetchSysInfo = async () => await send("GET_SYS_INFO")
const fetchMochiFiles = async () => await send("LIST_MOCHI")

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

const saveConfig = async () => await send("SET_CONFIG:" + JSON.stringify(config.value))
const reboot = async () => await send("REBOOT")
const deleteFile = async (name: string) => {
  if (confirm(t("mochi.delete") + " " + name + "?")) {
    await send("DELETE_FILE:" + config.value?.homePath + "/Mochi/" + name)
    setTimeout(fetchMochiFiles, 500)
  }
}

const handleSerialData = (e: any) => {
  const data = e.detail
  console.log(`RX: ${data}`)
  
  if (data.startsWith("CONF:")) {
    try { config.value = JSON.parse(data.substring(5)) } catch(e) {}
    return
  } 
  if (data.startsWith("SYS:")) {
    try { sysInfo.value = JSON.parse(data.substring(4)) } catch(e) {}
    return
  }
  if (data.startsWith("FILES:")) {
    try { mochiFiles.value = JSON.parse(data.substring(6)) } catch(e) {}
    return
  }

  // Fallback: Coi là log từ ESP
  const timestamp = new Date().toLocaleTimeString()
  logs.value.push(`[${timestamp}] ${data}`)
  if (logs.value.length > 50) logs.value.shift()
}

onMounted(() => {
  window.addEventListener("serial-data", handleSerialData)
})

onUnmounted(() => {
  window.removeEventListener("serial-data", handleSerialData)
})

const ramPercent = computed(() => {
  if (!sysInfo.value) return 0
  return Math.round(((sysInfo.value.ram_total - sysInfo.value.ram_free) / sysInfo.value.ram_total) * 100)
})

const flashPercent = computed(() => {
  if (!sysInfo.value) return 0
  return Math.round(((sysInfo.value.flash_total - sysInfo.value.flash_free) / sysInfo.value.flash_total) * 100)
})

const sdPercent = computed(() => {
  if (!sysInfo.value || !sysInfo.value.sd_ok) return 0
  return Math.round(((sysInfo.value.sd_total - sysInfo.value.sd_free) / sysInfo.value.sd_total) * 100)
})

const formatSize = (kb) => filesize(kb * 1024, { base: 2, standard: "jedec" })
</script>

<template>
  <div class="min-h-screen flex flex-col p-6 max-w-5xl mx-auto space-y-6">
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
        <Button v-if="!isConnected" :label="t('common.connect')" @click="connect" class="rounded-full font-bold" />
        <Button v-else :label="t('common.disconnect')" severity="danger" outlined @click="disconnect" class="rounded-full font-bold" />
      </div>
    </header>

    <main class="flex-grow grid grid-cols-12 gap-6">
      <!-- Left Panel: Sidebar-like Info -->
      <aside class="col-span-12 lg:col-span-4 space-y-6">
        <section v-if="isConnected && sysInfo" class="bg-gray-800 p-6 rounded-2xl border border-gray-700 space-y-6 shadow-xl">
           <div class="flex items-center justify-between">
              <h2 class="font-bold text-gray-300 flex items-center"><Cpu class="w-4 h-4 mr-2" /> {{ t('system.title') }}</h2>
              <Button icon="pi pi-refresh" text rounded @click="fetchSysInfo" class="w-8 h-8" />
           </div>

           <div class="space-y-4">
              <div class="p-4 bg-gray-900/50 rounded-xl border border-gray-700/50">
                 <div class="flex justify-between text-xs text-gray-400 mb-1">
                    <span>{{ t('system.ram') }}</span>
                    <span>{{ formatSize(sysInfo.ram_total - sysInfo.ram_free) }} / {{ formatSize(sysInfo.ram_total) }}</span>
                 </div>
                 <ProgressBar :value="ramPercent" class="h-2" />
              </div>

              <div class="p-4 bg-gray-900/50 rounded-xl border border-gray-700/50">
                 <div class="flex justify-between text-xs text-gray-400 mb-1">
                    <span>{{ t('system.flash') }}</span>
                    <span>{{ formatSize(sysInfo.flash_total - sysInfo.flash_free) }} / {{ formatSize(sysInfo.flash_total) }}</span>
                 </div>
                 <ProgressBar :value="flashPercent" color="#10b981" class="h-2" />
              </div>

              <!-- SD Card -->
              <div class="p-4 bg-gray-900/50 rounded-xl border border-gray-700/50">
                 <div class="flex justify-between text-xs text-gray-400 mb-1">
                    <span>{{ t('system.sdcard') }}</span>
                    <span v-if="sysInfo.sd_ok">{{ formatSize(sysInfo.sd_total - sysInfo.sd_free) }} / {{ formatSize(sysInfo.sd_total) }}</span>
                    <span v-else class="text-red-500 font-bold italic">{{ t('system.sd_not_found') }}</span>
                 </div>
                 <ProgressBar v-if="sysInfo.sd_ok" :value="sdPercent" color="#8b5cf6" class="h-2" />
                 <div v-else class="h-2 bg-gray-700 rounded-full opacity-30"></div>
              </div>

              <div class="p-4 bg-blue-500/10 border border-blue-500/30 rounded-xl flex items-start space-x-3">
                 <Database class="w-4 h-4 text-blue-400 mt-0.5" />
                 <p class="text-[10px] text-blue-200 leading-relaxed">{{ t('system.storage_notice') }}</p>
              </div>

              <div class="grid grid-cols-1 gap-2 text-sm">
                 <div class="flex justify-between py-2 border-b border-gray-700/50">
                    <span class="text-gray-400">{{ t('system.cpu') }}</span>
                    <span class="font-mono text-xs">{{ sysInfo.cpu }}</span>
                 </div>
                 <div class="flex justify-between py-2 border-b border-gray-700/50">
                    <span class="text-gray-400">Firmware</span>
                    <span class="text-xs">v1.2.0-stable</span>
                 </div>
              </div>
           </div>
        </section>

        <section v-else class="bg-gray-800 p-8 rounded-2xl border border-gray-700 flex flex-col items-center justify-center text-center space-y-4 h-64 shadow-xl">
           <Database class="w-12 h-12 text-gray-700" />
           <p class="text-sm text-gray-500">{{ isConnected ? "Loading..." : t('common.disconnected') }}</p>
        </section>
      </aside>

      <!-- Right Panel: Main Config -->
      <div class="col-span-12 lg:col-span-8 bg-gray-800 rounded-2xl border border-gray-700 shadow-xl overflow-hidden flex flex-col">
         <div v-if="!isConnected" class="flex-grow flex flex-col items-center justify-center space-y-4 p-12">
            <Monitor class="w-20 h-20 text-gray-700" />
            <p class="text-gray-400 font-medium">{{ t('common.disconnected') }}</p>
            <Button :label="t('common.connect')" icon="pi pi-link" @click="connect" class="p-button-lg px-8 rounded-xl" />
         </div>

         <Tabs v-else value="0" class="flex-grow flex flex-col">
            <TabList class="px-6 pt-2 bg-gray-800/50 border-b border-gray-700">
               <Tab value="0"><Wifi class="w-4 h-4 mr-2" /> {{ t('tabs.wifi') }}</Tab>
               <Tab value="1"><Monitor class="w-4 h-4 mr-2" /> {{ t('tabs.display') }}</Tab>
               <Tab value="2"><Droplets class="w-4 h-4 mr-2" /> {{ t('tabs.drink') }}</Tab>
               <Tab value="3"><Smile class="w-4 h-4 mr-2" /> {{ t('tabs.mochi') }}</Tab>
               <Tab value="4"><RefreshCw class="w-4 h-4 mr-2" /> {{ t('tabs.system') }}</Tab>
            </TabList>

            <TabPanels class="flex-grow p-8 overflow-y-auto">
               <TabPanel value="0">
                  <div v-if="config" class="space-y-6">
                    <div class="flex items-center justify-between mb-4">
                       <div>
                          <h2 class="text-xl font-bold">{{ t('tabs.wifi') }}</h2>
                          <p class="text-xs text-gray-500">Manage saved networks (Priority from top to bottom)</p>
                       </div>
                       <div class="flex space-x-2">
                          <Button icon="pi pi-plus" :label="t('common.add')" @click="config.wifi.push({ssid: '', pass: ''})" severity="success" size="small" />
                          <Button icon="pi pi-sync" @click="fetchConfig" text size="small" />
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
               </TabPanel>

               <TabPanel value="1">
                  <div v-if="config" class="space-y-8">
                    <div class="flex justify-between items-center p-6 bg-gray-900/30 rounded-2xl border border-gray-700">
                       <div class="space-y-1">
                          <h3 class="font-bold">Độ sáng màn hình</h3>
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
               </TabPanel>

               <TabPanel value="2">
                  <div v-if="config" class="space-y-8">
                     <div class="flex items-center justify-between p-6 bg-blue-500/10 rounded-2xl border border-blue-500/30">
                        <div>
                           <h2 class="text-xl font-bold text-blue-300">Drink Reminder</h2>
                           <p class="text-xs text-blue-400/60">Stay hydrated!</p>
                        </div>
                        <ToggleSwitch v-model="config.drink.enabled" />
                     </div>

                     <div class="grid grid-cols-1 md:grid-cols-3 gap-6">
                        <div class="flex flex-col space-y-2">
                           <label class="text-xs font-bold text-gray-500">Interval (minutes)</label>
                           <InputNumber v-model="config.drink.interval" :min="1" showButtons />
                        </div>
                        <div class="flex flex-col space-y-2">
                           <label class="text-xs font-bold text-gray-500">Start Hour</label>
                           <InputNumber v-model="config.drink.startHour" :min="0" :max="23" showButtons />
                        </div>
                        <div class="flex flex-col space-y-2">
                           <label class="text-xs font-bold text-gray-500">End Hour</label>
                           <InputNumber v-model="config.drink.endHour" :min="0" :max="23" showButtons />
                        </div>
                     </div>
                  </div>
               </TabPanel>

               <TabPanel value="3">
                  <div class="space-y-8">
                     <div class="flex items-center justify-between">
                        <h2 class="text-xl font-bold">{{ t('mochi.manage') }}</h2>
                        <div class="flex space-x-2">
                           <Button icon="pi pi-refresh" @click="fetchMochiFiles" text />
                           <Button :label="t('mochi.upload')" icon="pi pi-upload" severity="success" size="small" />
                        </div>
                     </div>

                     <div class="grid grid-cols-2 md:grid-cols-3 gap-4">
                        <div v-for="file in mochiFiles" :key="file" class="p-4 bg-gray-900/50 rounded-xl border border-gray-700 flex flex-col items-center space-y-3 group hover:border-blue-500/50 transition-all">
                           <div class="w-16 h-16 bg-gray-800 rounded-lg flex items-center justify-center text-gray-600">
                              <Smile v-if="file.includes('smile')" class="text-blue-400" />
                              <Smile v-else />
                           </div>
                           <span class="text-xs font-mono truncate w-full text-center">{{ file }}</span>
                           <Button icon="pi pi-trash" severity="danger" text rounded size="small" class="opacity-0 group-hover:opacity-100 transition-opacity" @click="deleteFile(file)" />
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
               </TabPanel>

               <TabPanel value="4">
                  <div v-if="config" class="space-y-8">
                     <div class="p-6 bg-yellow-500/5 border border-yellow-500/20 rounded-2xl flex items-start space-x-4">
                        <div class="p-2 bg-yellow-500/20 rounded-lg text-yellow-500"><RefreshCw class="w-5 h-5" /></div>
                        <div>
                           <h3 class="font-bold text-yellow-500">{{ t('system.gpio') }}</h3>
                           <p class="text-xs text-yellow-200/50">{{ t('system.warning') }}</p>
                        </div>
                     </div>

                     <div class="grid grid-cols-2 md:grid-cols-3 gap-6">
                        <div class="flex flex-col space-y-2">
                           <label class="text-[10px] font-bold text-gray-500 uppercase">Screen SDA</label>
                           <InputNumber v-model="config.pinScreenSDA" :min="-1" :max="48" />
                        </div>
                        <div class="flex flex-col space-y-2">
                           <label class="text-[10px] font-bold text-gray-500 uppercase">Screen SCL</label>
                           <InputNumber v-model="config.pinScreenSCL" :min="-1" :max="48" />
                        </div>
                        <div class="flex flex-col space-y-2">
                           <label class="text-[10px] font-bold text-gray-500 uppercase">Touch Sensor</label>
                           <InputNumber v-model="config.pinSensorTap" :min="-1" :max="48" />
                        </div>
                     </div>
                     <div class="pt-4 border-t border-gray-700/50">
                        <h4 class="text-sm font-bold text-gray-400 mb-3">Audio Pins</h4>
                        <div class="grid grid-cols-2 md:grid-cols-3 gap-6">
                           <div class="flex flex-col space-y-2">
                              <label class="text-[10px] font-bold text-gray-500 uppercase">DIN</label>
                              <InputNumber v-model="config.pinAudioDIN" :min="-1" :max="48" />
                           </div>
                           <div class="flex flex-col space-y-2">
                              <label class="text-[10px] font-bold text-gray-500 uppercase">LRC</label>
                              <InputNumber v-model="config.pinAudioLRC" :min="-1" :max="48" />
                           </div>
                           <div class="flex flex-col space-y-2">
                              <label class="text-[10px] font-bold text-gray-500 uppercase">BCLK</label>
                              <InputNumber v-model="config.pinAudioBCLK" :min="-1" :max="48" />
                           </div>
                        </div>
                     </div>
                     <div class="pt-4 border-t border-gray-700/50">
                        <h4 class="text-sm font-bold text-gray-400 mb-3">SD Card Pins</h4>
                        <div class="grid grid-cols-2 md:grid-cols-4 gap-6">
                           <div class="flex flex-col space-y-2">
                              <label class="text-[10px] font-bold text-gray-500 uppercase">CS</label>
                              <InputNumber v-model="config.pinSdCS" :min="-1" :max="48" />
                           </div>
                           <div class="flex flex-col space-y-2">
                              <label class="text-[10px] font-bold text-gray-500 uppercase">MOSI</label>
                              <InputNumber v-model="config.pinSdMOSI" :min="-1" :max="48" />
                           </div>
                           <div class="flex flex-col space-y-2">
                              <label class="text-[10px] font-bold text-gray-500 uppercase">CLK</label>
                              <InputNumber v-model="config.pinSdCLK" :min="-1" :max="48" />
                           </div>
                           <div class="flex flex-col space-y-2">
                              <label class="text-[10px] font-bold text-gray-500 uppercase">MISO</label>
                              <InputNumber v-model="config.pinSdMISO" :min="-1" :max="48" />
                           </div>
                        </div>
                     </div>

                     <div class="pt-6 border-t border-gray-700 space-y-6">
                        <div class="grid grid-cols-2 gap-6">
                           <div class="flex flex-col space-y-2">
                              <label class="text-xs font-bold text-gray-500">NTP Server</label>
                              <InputText v-model="config.ntpServer" />
                           </div>
                           <div class="flex flex-col space-y-2">
                              <label class="text-xs font-bold text-gray-500">GMT Offset (seconds)</label>
                              <InputNumber v-model="config.gmtOffset_sec" />
                           </div>
                           <div class="flex flex-col space-y-2">
                              <label class="text-xs font-bold text-gray-500">Daylight Offset (seconds)</label>
                              <InputNumber v-model="config.daylightOffset_sec" />
                           </div>
                        </div>
                        <div class="flex items-center justify-between p-4 bg-gray-900/30 rounded-lg">
                           <label class="font-medium text-sm">Enable WiFi</label>
                           <ToggleSwitch v-model="config.wifiEnabled" />
                        </div>
                        <div class="flex items-center justify-between p-4 bg-gray-900/30 rounded-lg">
                           <label class="font-medium text-sm">Enable Bluetooth</label>
                           <ToggleSwitch v-model="config.bluetoothEnabled" />
                        </div>
                        <div class="flex items-center justify-between p-4 bg-gray-900/30 rounded-lg">
                           <label class="font-medium text-sm">24-Hour Format</label>
                           <ToggleSwitch v-model="config.is24Hour" />
                        </div>
                        <div class="flex flex-col space-y-2 pt-4 mt-4 border-t border-gray-700">
                           <label class="text-xs font-bold text-gray-500">WiFi AP Name</label>
                           <InputText v-model="config.wifiAPName" />
                        </div>
                        <div class="flex flex-col space-y-2">
                           <label class="text-xs font-bold text-gray-500">Bluetooth Name</label>
                           <InputText v-model="config.bluetoothName" />
                        </div>
                        <div class="flex flex-col space-y-2">
                           <label class="text-xs font-bold text-gray-500">Weather Server URL</label>
                           <InputText v-model="config.weatherServer" />
                        </div>
                        <div class="flex flex-col space-y-2">
                           <label class="text-xs font-bold text-gray-500">Home Path</label>
                           <InputText v-model="config.homePath" />
                        </div>
                     </div>
                  </div>
               </TabPanel>
            </TabPanels>

            <div class="p-6 bg-gray-900/50 border-t border-gray-700 flex justify-end space-x-3">
               <Button :label="t('common.reboot')" severity="secondary" @click="reboot" text size="small" />
               <Button :label="t('common.save')" icon="pi pi-save" @click="saveConfig" class="px-8" />
            </div>
         </Tabs>
      </div>
    </main>

    <!-- Serial Log Section -->
    <footer v-if="isConnected" class="bg-gray-900 rounded-2xl border border-gray-700 overflow-hidden shadow-xl">
       <div class="bg-gray-800 px-4 py-2 border-b border-gray-700 flex justify-between items-center">
          <span class="text-xs font-bold text-gray-400 uppercase tracking-widest flex items-center">
             <div class="w-2 h-2 bg-green-500 rounded-full mr-2 animate-pulse"></div>
             ESP32 Serial Output
          </span>
          <Button icon="pi pi-trash" @click="logs = []" text size="small" />
       </div>
       <div class="p-4 h-48 overflow-y-auto font-mono text-[10px] space-y-1 bg-black/50">
          <div v-for="(log, i) in logs" :key="i" class="text-gray-300 border-l-2 border-blue-500/30 pl-2">
             {{ log }}
          </div>
          <div v-if="logs.length === 0" class="text-gray-600 italic">Waiting for logs...</div>
       </div>
    </footer>
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