<script setup lang="ts">
import { useI18n } from "vue-i18n"
import InputText from "primevue/inputtext"
import InputNumber from "primevue/inputnumber"
import ToggleSwitch from "primevue/toggleswitch"
import { RefreshCw } from "lucide-vue-next"
import type { AppConfig } from "../types/config"

defineProps<{
  config: AppConfig
}>()

const { t } = useI18n()
</script>

<template>
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
</template>
