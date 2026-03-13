#include "gif_player.h"

#include <SPIFFS.h>

#include "../config.h"
#include "../display.h"
#include "../filesystem.h"

static SemaphoreHandle_t gifPlayerMutex;

// ---------------------------------------------------------------------------
// Internal state
// ---------------------------------------------------------------------------
static File _file;
static bool _playing = false;
static uint8_t _frameCount = 0;
static uint16_t _width = 0;
static uint16_t _height = 0;
static uint16_t _delays[QGIF_MAX_FRAMES];
static uint8_t *_frameBuf = nullptr;  // dynamically allocated
static uint16_t _frameBufSize = 0;    // size in bytes
static uint8_t _currentFrame = 0;
static unsigned long _lastFrameMs = 0;
static uint32_t _dataOffset = 0;  // byte offset to first frame in file
static String _currentFile;
static String _requestedFile;
static bool _fileChanged = false;

// ---------------------------------------------------------------------------
// Private helpers
// ---------------------------------------------------------------------------

// Free the dynamic frame buffer if allocated
static void _freeFrameBuf() {
  if (_frameBuf) {
    free(_frameBuf);
    _frameBuf = nullptr;
    _frameBufSize = 0;
  }
}

// Open a .qgif file, parse header + delays, prepare for frame streaming.
static bool _openFile(const String &filename) {
  if (_file) _file.close();
  _playing = false;
  _freeFrameBuf();  // free old buffer

  String path = "/" + filename;
  _file = getFile(path, FILE_READ);
  if (!_file) {
    Serial.println("gifPlayer: cannot open " + path);
    return false;
  }

  // --- Read 5-byte header ---
  uint8_t hdr[QGIF_HEADER_SIZE];
  if (_file.read(hdr, QGIF_HEADER_SIZE) != QGIF_HEADER_SIZE) {
    _file.close();
    return false;
  }

  _frameCount = hdr[0];
  _width = hdr[1] | ((uint16_t)hdr[2] << 8);
  _height = hdr[3] | ((uint16_t)hdr[4] << 8);

  if (_frameCount == 0 || _frameCount > QGIF_MAX_FRAMES) {
    Serial.printf("gifPlayer: invalid frame count %u\n", _frameCount);
    _file.close();
    return false;
  }

  // Calculate required frame buffer size (bytes per frame)
  uint16_t frameBytes = ((_width + 7) / 8) * _height;
  if (frameBytes == 0) {
    Serial.println("gifPlayer: invalid frame dimensions");
    _file.close();
    return false;
  }

  // Allocate dynamic buffer for one frame
  _frameBuf = (uint8_t *)malloc(frameBytes);
  if (!_frameBuf) {
    Serial.println("gifPlayer: out of memory for frame buffer");
    _file.close();
    return false;
  }
  _frameBufSize = frameBytes;

  // --- Read delays array ---
  uint16_t delayBytes = (uint16_t)_frameCount * 2;
  uint8_t delayBuf[QGIF_MAX_FRAMES * 2];
  if (_file.read(delayBuf, delayBytes) != delayBytes) {
    _file.close();
    _freeFrameBuf();
    return false;
  }
  for (uint8_t i = 0; i < _frameCount; i++) {
    _delays[i] = delayBuf[i * 2] | ((uint16_t)delayBuf[i * 2 + 1] << 8);
  }

  _dataOffset = QGIF_HEADER_SIZE + delayBytes;
  _currentFrame = 0;
  _lastFrameMs = 0;  // render first frame immediately
  _currentFile = filename;
  _playing = true;
  return true;
}

// Read one frame into _frameBuf by seeking to its offset.
static bool _readFrame(uint8_t idx) {
  if (!_frameBuf) return false;
  uint32_t off = _dataOffset + (uint32_t)idx * _frameBufSize;
  if (!_file.seek(off)) return false;
  return _file.read(_frameBuf, _frameBufSize) == _frameBufSize;
}

// ---------------------------------------------------------------------------
// Public API
// ---------------------------------------------------------------------------

void gifPlayerSetFile(const String &filename) {
  if (gifPlayerMutex) xSemaphoreTake(gifPlayerMutex, portMAX_DELAY);
  _requestedFile = filename;
  _fileChanged = true;
  if (gifPlayerMutex) xSemaphoreGive(gifPlayerMutex);
}

String gifPlayerGetCurrentFile() {
  if (gifPlayerMutex) xSemaphoreTake(gifPlayerMutex, portMAX_DELAY);
  String f = _currentFile;
  if (gifPlayerMutex) xSemaphoreGive(gifPlayerMutex);
  return f;
}

static void gifRenderFrame(uint8_t *frameData, uint16_t width,
                           uint16_t height) {
  // Invert polarity unless Negative GIF mode is on
  if (config.mochiNegative) {
    uint16_t dataLen = ((width + 7) / 8) * height;
    for (uint16_t i = 0; i < dataLen; i++) frameData[i] ^= 0xFF;
  }

  uint16_t screenWidth = config.screenWidth;
  uint16_t screenHeight = config.screenHeight;

  int16_t x = (screenWidth - width) / 2;
  int16_t y = (screenHeight - height) / 2;

  u8g2.clearBuffer();
  u8g2.drawBitmap(x, y, (width + 7) / 8, height, frameData);
  sendBuffer();
}
void gifPlayerTick() {
  // Handle pending file-change request
  if (_fileChanged) {
    _fileChanged = false;
    if (_requestedFile.length() > 0) {
      _openFile(_requestedFile);
    } else {
      if (_file) _file.close();
      _playing = false;
      _currentFile = "";
      _freeFrameBuf();  // free buffer when stopped
    }
  }

  if (!_playing || !_frameBuf) return;

  // Frame timing
  uint16_t delayMs = _delays[_currentFrame] / config.mochiSpeedDivisor;
  if (delayMs < 1) delayMs = 1;
  if (millis() - _lastFrameMs < delayMs) return;

  // Read frame from flash and render
  if (_readFrame(_currentFrame)) {
    gifRenderFrame(_frameBuf, _width, _height);
  }

  _lastFrameMs = millis();
  _currentFrame++;
  if (_currentFrame >= _frameCount) {
    _currentFrame = 0;  // loop back to first frame
  }
}
