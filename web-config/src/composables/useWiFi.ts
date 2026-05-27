import { ref } from "vue"

export function useWiFi() {
  const isConnected = ref(false)
  const baseUrl = ref(localStorage.getItem("mochi_host") || "http://dasai-mochi-shin.local")
  const devices = ref<{ name: string, ip: string, url: string }[]>([])
  const isScanning = ref(false)

  const setHost = (url: string) => {
    baseUrl.value = url.endsWith("/") ? url.slice(0, -1) : url
    localStorage.setItem("mochi_host", baseUrl.value)
  }

  const connect = async (url?: string) => {
    if (url) setHost(url)
    try {
      const resp = await fetch(`${baseUrl.value}/api/info`)
      if (resp.ok) {
        isConnected.value = true
        return true
      }
    } catch (e) {
      console.error("Failed to connect to Mochi", e)
      isConnected.value = false
    }
    return false
  }

  const disconnect = () => {
    isConnected.value = false
  }

  const scan = async () => {
    isScanning.value = true
    devices.value = []
    
    // Try popular local names
    const targets = [
      "http://dasai-mochi-shin.local",
      "http://mochishin.local",
      "http://mochi.local"
    ]

    for (const target of targets) {
      try {
        const controller = new AbortController()
        const timeoutId = setTimeout(() => controller.abort(), 2000)
        const resp = await fetch(`${target}/api/info`, { signal: controller.signal })
        clearTimeout(timeoutId)
        
        if (resp.ok) {
          const info = await resp.json()
          devices.value.push({
            name: info.module || "Mochi Device",
            ip: target.replace("http://", ""),
            url: target
          })
        }
      } catch (e) {
        // Ignore failures
      }
    }
    
    isScanning.value = false
  }

  const request = async (path: string, method = "GET", body?: any) => {
    const url = `${baseUrl.value}${path}`
    const options: RequestInit = {
      method,
      headers: body ? { "Content-Type": "application/json" } : {}
    }
    if (body) options.body = JSON.stringify(body)

    const resp = await fetch(url, options)
    if (!resp.ok) throw new Error(`HTTP error ${resp.status}`)
    return await resp.json()
  }

  const uploadFile = async (file: File) => {
    const formData = new FormData()
    formData.append("file", file)
    return await fetch(`${baseUrl.value}/api/files/upload`, {
      method: "POST",
      body: formData
    })
  }

  return { isConnected, baseUrl, devices, isScanning, connect, disconnect, scan, request, uploadFile }
}
