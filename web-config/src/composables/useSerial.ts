import { ref } from "vue"

export function useSerial() {
  const port = ref<any>(null)
  const isConnected = ref(false)
  const writer = ref<any>(null)

  const connect = async () => {
    try {
      // @ts-expect-error Web Serial API not in TS types
      port.value = await navigator.serial.requestPort()
      await port.value.open({ baudRate: 115200 })
      isConnected.value = true

      writer.value = port.value.writable.getWriter()
      listen()
    } catch (e) {
      console.error("Connection failed", e)
    }
  }

  const disconnect = async () => {
    if (port.value) {
      await writer.value.releaseLock()
      await port.value.close()
      isConnected.value = false
    }
  }

  const send = async (data: string) => {
    if (writer.value) {
      // Debug log
      console.log("Serial TX:", data)
      const encoder = new TextEncoder()
      await writer.value.write(encoder.encode(data + "\n"))
    }
  }

  const listen = async () => {
    while (port.value && port.value.readable) {
      const textDecoderStream = new TextDecoderStream()
      port.value.readable.pipeTo(textDecoderStream.writable)
      const readerStream = textDecoderStream.readable.getReader()

      try {
        let buffer = ""
        while (true) {
          const { value, done } = await readerStream.read()
          if (done) break

          buffer += value
          const lines = buffer.split("\n")
          // Keep the last incomplete line in buffer
          buffer = lines.pop() || ""

          for (const line of lines) {
            const trimmedLine = line.trim()
            if (trimmedLine) {
              window.dispatchEvent(new CustomEvent("serial-data", { detail: trimmedLine }))
            }
          }
        }
      } catch (error) {
        console.error("Serial read error", error)
      } finally {
        readerStream.releaseLock()
      }
    }
  }

  return { isConnected, connect, disconnect, send }
}
