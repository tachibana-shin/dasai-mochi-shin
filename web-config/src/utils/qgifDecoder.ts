export interface QgifData {
  frameCount: number
  width: number
  height: number
  delays: number[]
  frames: Uint8Array[]
}

export function parseQgif(buf: ArrayBuffer): QgifData | null {
  const view = new DataView(buf)
  if (buf.byteLength < 5) return null

  const firstByte = view.getUint8(0)
  let frameCount: number
  let width: number
  let height: number
  let headerSize: number

  if (firstByte === 0) {
    if (buf.byteLength < 7) return null
    frameCount = view.getUint16(1, true)
    width = view.getUint16(3, true)
    height = view.getUint16(5, true)
    headerSize = 7
  } else {
    frameCount = firstByte
    width = view.getUint16(1, true)
    height = view.getUint16(3, true)
    headerSize = 5
  }

  if (frameCount === 0 || width === 0 || height === 0) return null

  const frameBufSize = Math.ceil(width / 8) * height
  const expectedSize = headerSize + frameCount * 2 + frameCount * frameBufSize
  if (buf.byteLength < expectedSize) return null

  const delays: number[] = []
  for (let i = 0; i < frameCount; i++) {
    delays.push(view.getUint16(headerSize + i * 2, true))
  }

  const framesOffset = headerSize + frameCount * 2
  const frames: Uint8Array[] = []
  for (let i = 0; i < frameCount; i++) {
    frames.push(new Uint8Array(buf, framesOffset + i * frameBufSize, frameBufSize))
  }

  return { frameCount, width, height, delays, frames }
}

export function renderQgifFrame(
  ctx: CanvasRenderingContext2D,
  frame: Uint8Array,
  width: number,
  height: number,
  scale: number,
) {
  const cw = width * scale
  const ch = height * scale
  const imgData = ctx.createImageData(cw, ch)
  const data = imgData.data

  for (let y = 0; y < height; y++) {
    for (let x = 0; x < width; x++) {
      const byteIdx = y * Math.ceil(width / 8) + Math.floor(x / 8)
      const bitIdx = 7 - (x % 8)
      const on = (frame[byteIdx] >> bitIdx) & 1
      const lit = on ? 0 : 1
      const val = lit ? 255 : 0

      for (let sy = 0; sy < scale; sy++) {
        for (let sx = 0; sx < scale; sx++) {
          const px = x * scale + sx
          const py = y * scale + sy
          const idx = (py * cw + px) * 4
          data[idx] = val
          data[idx + 1] = val
          data[idx + 2] = val
          data[idx + 3] = 255
        }
      }
    }
  }

  ctx.putImageData(imgData, 0, 0)
}
