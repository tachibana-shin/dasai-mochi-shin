const ui = {
  themeBtn: document.getElementById('themeBtn'),
  versionEl: document.getElementById('version'),
  timestampEl: document.getElementById('timestamp'),
  firmwareSizeEl: document.getElementById('firmware-size'),
  firmwareMd5El: document.getElementById('firmware-md5'),
  refreshBtn: document.getElementById('refreshBtn'),
  installBtn: document.getElementById('installBtn'),
  eraseCheckbox: document.getElementById('eraseCheckbox'),
  logCard: document.getElementById('logCard'),
  logOutput: document.getElementById('logOutput'),
}

let manifestUrl = 'manifest.json'
let currentVersion = ''

document.addEventListener('DOMContentLoaded', () => {
  const savedTheme = localStorage.getItem('theme')
  if (savedTheme === 'light') document.documentElement.classList.add('light-mode')
  syncThemeButton()
  ui.themeBtn.addEventListener('click', toggleTheme)

  loadLatestVersion()
  ui.refreshBtn.addEventListener('click', loadLatestVersion)
  ui.eraseCheckbox.addEventListener('change', updateManifest)
  ui.installBtn.addEventListener('state-changed', onInstallStateChanged)
})

function toggleTheme() {
  document.documentElement.classList.toggle('light-mode')
  const isLight = document.documentElement.classList.contains('light-mode')
  localStorage.setItem('theme', isLight ? 'light' : 'dark')
  syncThemeButton()
}

function syncThemeButton() {
  const isLight = document.documentElement.classList.contains('light-mode')
  ui.themeBtn.classList.toggle('is-light', isLight)
}

async function loadLatestVersion() {
  try {
    ui.refreshBtn.disabled = true
    ui.versionEl.textContent = 'Loading...'
    ui.timestampEl.textContent = 'Loading...'
    ui.firmwareSizeEl.textContent = 'Loading...'
    ui.firmwareMd5El.textContent = 'Loading...'

    const res = await fetch('latest.json')
    if (!res.ok) throw new Error(`Failed to fetch latest.json: ${res.status}`)
    const data = await res.json()

    currentVersion = data.version || 'Unknown'
    ui.versionEl.textContent = currentVersion
    ui.timestampEl.textContent = data.timestamp ? formatDate(data.timestamp) : 'Unknown'

    const fw = data.files && data.files['firmware.bin']
    if (fw) {
      ui.firmwareSizeEl.textContent = typeof fw.size === 'number' ? formatBytes(fw.size) : 'Unknown'
      ui.firmwareMd5El.textContent = typeof fw.md5 === 'string' && fw.md5.length >= 16
        ? fw.md5.substring(0, 16) + '...'
        : 'Unknown'
    } else {
      ui.firmwareSizeEl.textContent = 'Unknown'
      ui.firmwareMd5El.textContent = 'Unknown'
    }

    updateManifest()
  } catch (err) {
    console.error('Load version failed:', err)
    ui.versionEl.textContent = '[ERR] Load failed'
  } finally {
    ui.refreshBtn.disabled = false
  }
}

function updateManifest() {
  const eraseFirst = ui.eraseCheckbox.checked
  const manifest = {
    name: 'Dasai Mochi Shin',
    version: currentVersion,
    new_install_prompt_erase: eraseFirst,
    builds: [
      {
        chipFamily: 'ESP32-C3',
        parts: [
          { path: 'bootloader.bin', offset: 0 },
          { path: 'partitions.bin', offset: 0x8000 },
          { path: 'firmware.bin', offset: 0x10000 },
          { path: 'littlefs.bin', offset: 0x210000 },
        ],
      },
    ],
  }

  if (manifestUrl.startsWith('blob:')) URL.revokeObjectURL(manifestUrl)
  const blob = new Blob([JSON.stringify(manifest)], { type: 'application/json' })
  manifestUrl = URL.createObjectURL(blob)
  ui.installBtn.setAttribute('manifest', manifestUrl)
}

function onInstallStateChanged(e) {
  const state = e.detail.state
  const msg = e.detail.message || ''

  if (state === 'initializing' || state === 'uploading' || state === 'finished' || state === 'error') {
    ui.logCard.style.display = 'block'
    if (msg) {
      ui.logOutput.textContent += msg + '\n'
      ui.logOutput.scrollTop = ui.logOutput.scrollHeight
    }
  }

  if (state === 'finished') {
    ui.logOutput.textContent += '✓ Flash complete!\n'
  }

  if (state === 'error') {
    ui.logOutput.textContent += '✗ Error: ' + msg + '\n'
  }
}

function formatBytes(bytes) {
  if (bytes === 0) return '0 B'
  const k = 1024
  const sizes = ['B', 'KB', 'MB', 'GB']
  const i = Math.floor(Math.log(bytes) / Math.log(k))
  return Math.round((bytes / Math.pow(k, i)) * 100) / 100 + ' ' + sizes[i]
}

function formatDate(isoString) {
  if (!isoString) return null
  const date = new Date(isoString)
  if (Number.isNaN(date.getTime())) return null
  const y = date.getFullYear()
  const m = String(date.getMonth() + 1).padStart(2, '0')
  const d = String(date.getDate()).padStart(2, '0')
  const h = String(date.getHours()).padStart(2, '0')
  const min = String(date.getMinutes()).padStart(2, '0')
  const s = String(date.getSeconds()).padStart(2, '0')
  return `${y}-${m}-${d} ${h}:${min}:${s}`
}
