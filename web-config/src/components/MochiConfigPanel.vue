<script setup lang="ts">
import { filesize } from "filesize"
import { Smile, X } from "lucide-vue-next"
import Button from "primevue/button"
import InputNumber from "primevue/inputnumber"
import ToggleSwitch from "primevue/toggleswitch"
import { computed, nextTick, onUnmounted, ref } from "vue"
import { useI18n } from "vue-i18n"

import type { AppConfig } from "../types/config"

import { baseUrl } from "../composables/useWiFi"
import { parseQgif, renderQgifFrame } from "../utils/qgifDecoder"
import type { QgifData } from "../utils/qgifDecoder"

const props = defineProps<{
  config: AppConfig
  mochiFiles: { name: string; size: number }[]
  isUploading: boolean
}>()

const emit = defineEmits<{
  refresh: []
  upload: []
  delete: [name: string]
  fileSelected: [event: Event]
}>()

const { t } = useI18n()
const fileInput = ref<HTMLInputElement | null>(null)

function triggerUpload() {
  fileInput.value?.click()
}

// ── Hover Preview ──
const hoveredFile = ref<string | null>(null)
const visibleFiles = ref<Set<string>>(new Set())
const qgifCache = new Map<string, QgifData>()
const canvasRefs = new Map<string, HTMLCanvasElement>()
const animTimers = new Map<string, number>()

const previewScale = computed(() => {
  const w = props.config.screenWidth || 128
  const h = props.config.screenHeight || 64
  const maxPx = 96
  return Math.max(1, Math.floor(Math.min(maxPx / w, maxPx / h)))
})

let hoverTimer: number | undefined

function startPreview(name: string) {
  clearTimeout(hoverTimer)
  hoverTimer = window.setTimeout(async () => {
    if (!visibleFiles.value.has(name)) return
    hoveredFile.value = name
    await nextTick()
    loadAndPlay(name)
  }, 300)
}

function stopPreview(name: string) {
  clearTimeout(hoverTimer)
  if (hoveredFile.value === name) {
    hoveredFile.value = null
    stopAnim(name)
  }
}

async function loadAndPlay(name: string) {
  if (qgifCache.has(name)) {
    startAnim(name)
    return
  }
  try {
    const url = `${baseUrl.value}/Mochi/${encodeURIComponent(name)}`
    const res = await fetch(url)
    const buf = await res.arrayBuffer()
    const qgif = parseQgif(buf)
    if (!qgif) return
    qgifCache.set(name, qgif)
    if (hoveredFile.value === name) startAnim(name)
  } catch {}
}

function startAnim(name: string) {
  const qgif = qgifCache.get(name)
  const canvas = canvasRefs.get(name)
  if (!qgif || !canvas) return

  const ctx = canvas.getContext("2d")
  if (!ctx) return

  const scale = previewScale.value
  canvas.width = qgif.width * scale
  canvas.height = qgif.height * scale

  let frameIdx = 0
  let timer: number

  function tick() {
    if (hoveredFile.value !== name) return
    renderQgifFrame(ctx, qgif!.frames[frameIdx], qgif!.width, qgif!.height, scale)
    const delay = Math.max(qgif!.delays[frameIdx] || 100, 16)
    frameIdx = (frameIdx + 1) % qgif!.frameCount
    timer = window.setTimeout(tick, delay)
  }

  animTimers.set(name, timer)
  tick()
}

function stopAnim(name: string) {
  const timer = animTimers.get(name)
  if (timer !== undefined) {
    clearTimeout(timer)
    animTimers.delete(name)
  }
}

function setCanvasRef(name: string, el: any) {
  if (el) canvasRefs.set(name, el as HTMLCanvasElement)
  else canvasRefs.delete(name)
}

// ── IntersectionObserver (lazy) ──
let observer: IntersectionObserver | null = null

function observeCard(el: HTMLElement | null) {
  if (!el) return
  const name = el.dataset.mochiName
  if (!name) return

  if (!observer) {
    observer = new IntersectionObserver((entries) => {
      const set = new Set(visibleFiles.value)
      for (const entry of entries) {
        const n = (entry.target as HTMLElement).dataset.mochiName
        if (n) {
          if (entry.isIntersecting) set.add(n)
          else set.delete(n)
        }
      }
      visibleFiles.value = set
    }, { rootMargin: "100px" })
  }
  observer.observe(el)
}

onUnmounted(() => {
  observer?.disconnect()
  for (const t of animTimers.values()) clearTimeout(t)
  animTimers.clear()
})

// ── Modal Preview ──
const showPreview = ref(false)
const previewUrl = ref("")
const modalCanvas = ref<HTMLCanvasElement | null>(null)
let modalTimer: number | undefined

function previewFile(name: string) {
  showPreview.value = true
  previewUrl.value = name
  const url = `${baseUrl.value}/Mochi/${encodeURIComponent(name)}`

  fetch(url)
    .then((r) => r.arrayBuffer())
    .then((buf) => {
      const qgif = parseQgif(buf)
      if (!qgif) return
      const canvas = modalCanvas.value
      if (!canvas) return
      const scale = 3
      canvas.width = qgif.width * scale
      canvas.height = qgif.height * scale
      const ctx = canvas.getContext("2d")
      if (!ctx) return

      let frame = 0
      function tick() {
        if (!ctx || !qgif) return
        renderQgifFrame(ctx, qgif.frames[frame], qgif.width, qgif.height, scale)
        const delay = Math.max(qgif.delays[frame] || 100, 16)
        frame = (frame + 1) % qgif.frameCount
        modalTimer = window.setTimeout(tick, delay)
      }
      tick()
    })
    .catch(() => {})
}

function closePreview() {
  showPreview.value = false
  if (modalTimer !== undefined) clearTimeout(modalTimer)
}

defineExpose({ fileInput })
</script>

<template>
  <div class="space-y-8">
    <div class="flex items-center justify-between">
      <h2 class="text-xl font-bold">{{ t("mochi.manage") }}</h2>
      <div class="flex space-x-2">
        <Button icon="pi pi-refresh" @click="emit('refresh')" text />
        <input
          type="file"
          ref="fileInput"
          class="hidden"
          @change="emit('fileSelected', $event)"
          accept=".qgif,.qgif+"
        />
        <Button
          :label="isUploading ? 'Uploading...' : t('mochi.upload')"
          icon="pi pi-upload"
          severity="success"
          size="small"
          @click="triggerUpload"
          :loading="isUploading"
        />
      </div>
    </div>

    <div class="grid grid-cols-2 md:grid-cols-3 gap-4">
      <div
        v-for="file in mochiFiles"
        :key="file.name"
        :ref="observeCard"
        :data-mochi-name="file.name"
        class="p-4 bg-gray-900/50 rounded-xl border border-gray-700 flex flex-col items-center space-y-3 group hover:border-blue-500/50 transition-all relative"
        @mouseenter="startPreview(file.name)"
        @mouseleave="stopPreview(file.name)"
      >
        <div class="w-16 h-16 bg-gray-800 rounded-lg flex items-center justify-center text-gray-600 overflow-hidden">
          <canvas
            v-if="hoveredFile === file.name && qgifCache.has(file.name)"
            :ref="(el: any) => setCanvasRef(file.name, el)"
            class="mx-auto"
            style="image-rendering: pixelated"
          />
          <Smile v-else :class="file.name.includes('smile') ? 'text-blue-400' : ''" />
        </div>
        <span class="text-xs font-mono truncate w-full text-center">{{ file.name }}</span>
        <span class="text-[10px] text-gray-500">{{ filesize(file.size) }}</span>

        <div class="absolute top-2 right-2 flex space-x-1 opacity-0 group-hover:opacity-100 transition-opacity">
          <Button icon="pi pi-pencil" severity="info" text rounded size="small" />
          <Button icon="pi pi-eye" severity="help" text rounded size="small" @click.stop="previewFile(file.name)" />
          <Button icon="pi pi-trash" severity="danger" text rounded size="small" @click="emit('delete', file.name)" />
        </div>
      </div>
    </div>

    <div v-if="config" class="pt-8 border-t border-gray-700 space-y-6">
      <h2 class="text-lg font-bold">{{ t("mochi.animation") }}</h2>
      <div class="grid grid-cols-2 gap-6">
        <div class="flex flex-col space-y-2">
          <label class="text-xs font-bold text-gray-500">{{ t("mochi.animation") }} Divisor</label>
          <InputNumber v-model="config.mochiSpeedDivisor" :min="1" :max="10" showButtons />
        </div>
        <div class="flex flex-col space-y-2">
          <label class="text-xs font-bold text-gray-500">{{ t("mochi.interval") }} (ms)</label>
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

    <div
      v-if="showPreview"
      class="fixed inset-0 z-50 flex items-center justify-center bg-black/70"
      @click.self="closePreview"
    >
      <div class="bg-gray-900 p-6 rounded-2xl border border-gray-700 shadow-2xl relative">
        <button
          @click="closePreview"
          class="absolute -top-3 -right-3 w-8 h-8 bg-gray-800 rounded-full border border-gray-600 flex items-center justify-center hover:bg-gray-700"
        >
          <X class="w-4 h-4" />
        </button>
        <canvas ref="modalCanvas" class="mx-auto" style="image-rendering: pixelated" />
        <p class="text-xs text-gray-500 text-center mt-3">{{ previewUrl }}</p>
      </div>
    </div>
  </div>
</template>
