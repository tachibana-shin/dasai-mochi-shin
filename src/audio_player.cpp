#include "audio_player.h"

#include "config.h"
#include "driver/i2s.h"
#include "filesystem.h"

static bool _isPlaying = false;
static TaskHandle_t _audioTaskHandle = NULL;

struct AudioSource {
  const uint8_t* data;
  uint32_t size;
  fs::File file;
  uint32_t sampleRate;
  uint8_t channels;
  bool isFile;
};

static AudioSource _currentSource;

// Forward declaration for volume adjustment
void applyVolume(int16_t* samples, size_t num_samples, int volume);

void audioTask(void* pvParameters) {
  i2s_config_t i2s_config = {
    .mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_TX),
    .sample_rate = _currentSource.sampleRate,
    .bits_per_sample = I2S_BITS_PER_SAMPLE_16BIT,
    .channel_format = (_currentSource.channels == 2) ? I2S_CHANNEL_FMT_RIGHT_LEFT : I2S_CHANNEL_FMT_ONLY_LEFT,
    .communication_format = I2S_COMM_FORMAT_STAND_MSB,
    .intr_alloc_flags = ESP_INTR_FLAG_LEVEL1,
    .dma_buf_count = 8,
    .dma_buf_len = 64,
    .use_apll = false,
    .tx_desc_auto_clear = true,
    .fixed_mclk = 0
  }; 

  i2s_pin_config_t pin_config = {
    .bck_io_num = config.pinAudioBCLK,
    .ws_io_num = config.pinAudioLRC,
    .data_out_num = config.pinAudioDIN,
    .data_in_num = I2S_PIN_NO_CHANGE
  };

  i2s_driver_install(I2S_NUM_0, &i2s_config, 0, NULL);
  i2s_set_pin(I2S_NUM_0, &pin_config);
  i2s_set_clk(I2S_NUM_0, _currentSource.sampleRate, I2S_BITS_PER_SAMPLE_16BIT, (_currentSource.channels == 2) ? I2S_CHANNEL_STEREO : I2S_CHANNEL_MONO);

  size_t bytes_written;
  int16_t sample_buffer[128];

  if (_currentSource.isFile) {
    fs::File file = _currentSource.file;
    if (file) {
      char magic[4];
      file.readBytes(magic, 4);
      if (strncmp(magic, "QWAV", 4) == 0) {
        uint32_t rate; uint8_t chans; uint8_t bits;
        file.read((uint8_t*)&rate, 4);
        file.read(&chans, 1);
        file.read(&bits, 1);
        _currentSource.sampleRate = rate;
        _currentSource.channels = chans;
        i2s_set_clk(I2S_NUM_0, _currentSource.sampleRate, I2S_BITS_PER_SAMPLE_16BIT, (_currentSource.channels == 2) ? I2S_CHANNEL_STEREO : I2S_CHANNEL_MONO);
      } else if (strncmp(magic, "RIFF", 4) == 0) {
        file.seek(44);
      } else {
        file.seek(0);
      }

      while (file.available() && _isPlaying) {
        size_t bytes_to_read = 64 * _currentSource.channels * sizeof(int16_t);
        size_t bytes_read = file.read((uint8_t*)sample_buffer, bytes_to_read);
        if (bytes_read == 0) break;
        
        applyVolume(sample_buffer, bytes_read / sizeof(int16_t), config.audio.volume);
        i2s_write(I2S_NUM_0, sample_buffer, bytes_read, &bytes_written, portMAX_DELAY);
      }
      file.close();
    }
  } else {
    // Memory playback
    const int16_t* ptr = (const int16_t*)_currentSource.data;
    uint32_t remaining_bytes = _currentSource.size;
    
    while (remaining_bytes > 0 && _isPlaying) {
      size_t bytes_to_read = remaining_bytes > (128 * sizeof(int16_t)) ? (128 * sizeof(int16_t)) : remaining_bytes;
      memcpy(sample_buffer, ptr, bytes_to_read);
      
      applyVolume(sample_buffer, bytes_to_read / sizeof(int16_t), config.audio.volume);
      i2s_write(I2S_NUM_0, sample_buffer, bytes_to_read, &bytes_written, portMAX_DELAY);
      
      ptr += (bytes_written / sizeof(int16_t));
      remaining_bytes -= bytes_written;
    }
  }

  i2s_zero_dma_buffer(I2S_NUM_0);
  i2s_driver_uninstall(I2S_NUM_0);

  _isPlaying = false;
  _audioTaskHandle = NULL;
  vTaskDelete(NULL);
}

void audioInit() {}

void audioPlayFile(const fs::File& file) {
  if (_isPlaying || _audioTaskHandle != NULL) {
    audioStop();
    int timeout = 100;
    while ((_isPlaying || _audioTaskHandle != NULL) && timeout-- > 0) delay(5);
  }

  _currentSource.file = file;
  _currentSource.isFile = true;
  _isPlaying = true;

  xTaskCreate(audioTask, "audioTask", 4096, NULL, 5, &_audioTaskHandle);
}

static void audioPlayMemory(const uint8_t* data, uint32_t size, uint32_t sampleRate, uint8_t channels) {
  if (_isPlaying || _audioTaskHandle != NULL) {
    audioStop();
    int timeout = 100;
    while ((_isPlaying || _audioTaskHandle != NULL) && timeout-- > 0) delay(5);
  }

  _currentSource.data = data;
  _currentSource.size = size;
  _currentSource.sampleRate = sampleRate;
  _currentSource.channels = channels;
  _currentSource.isFile = false;
  _isPlaying = true;

  xTaskCreate(audioTask, "audioTask", 4096, NULL, 5, &_audioTaskHandle);
}

void audioPlayDefault(SoundType type) {
  String path = "";
  bool enabled = false;

  switch (type) {
    case SOUND_CLICK:
      path = config.customClickSoundPath;
      enabled = config.audio.notifyEnabled;
      break;
    case SOUND_DRINK:
      path = config.customDrinkSoundPath;
      enabled = config.audio.notifyEnabled;
      break;
    case SOUND_ALARM:
      path = config.customAlarmSoundPath;
      enabled = config.audio.alarmEnabled;
      break;
    case SOUND_NOTIFY:
      path = config.customNotifySoundPath;
      enabled = config.audio.notifyEnabled;
      break;
  }

  if (enabled && path != "") {
    fs::File file = getFile(config.homePath + "/" + path, FILE_READ);
      if (file) {
        audioPlayFile(file);
      } else {
        Serial.println("Sound file not found: " + path);
      
    }
  }
}

void audioStop() { _isPlaying = false; }

bool isAudioPlaying() { return _isPlaying; }

void applyVolume(int16_t* samples, size_t num_samples, int volume) {
  float gain = (float)volume / 21.0;
  for (size_t i = 0; i < num_samples; ++i) {
    samples[i] = (int16_t)(samples[i] * gain);
  }
}
