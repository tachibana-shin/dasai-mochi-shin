#include "audio_player.h"

#include "config.h"
#include "driver/i2s_std.h"
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
  size_t bytes_written;
  i2s_chan_handle_t local_tx_handle = NULL;

  if (_currentSource.isFile) {
    fs::File file = _currentSource.file;
    if (file) {
      char magic[4];
      file.readBytes(magic, 4);
      if (strncmp(magic, "QWAV", 4) == 0) {
        // Read QWAV header (10 bytes total, 4 already read)
        uint32_t rate;
        uint8_t chans;
        uint8_t bits;
        file.read((uint8_t*)&rate, 4);
        file.read(&chans, 1);
        file.read(&bits, 1);
        _currentSource.sampleRate = rate;
        _currentSource.channels = chans;
      } else if (strncmp(magic, "RIFF", 4) == 0) {
        // Standard WAV logic (skip 44 bytes header)
        file.seek(44);
      } else {
        file.seek(0);
      }

      // Initialize I2S *after* potentially updating rate/chans from header
      i2s_chan_config_t chan_cfg = I2S_CHANNEL_DEFAULT_CONFIG(I2S_NUM_0, I2S_ROLE_MASTER);
      if (i2s_new_channel(&chan_cfg, &local_tx_handle, NULL) == ESP_OK) {
        i2s_slot_mode_t slot_mode = (_currentSource.channels == 2) ? I2S_SLOT_MODE_STEREO : I2S_SLOT_MODE_MONO;
        i2s_std_slot_config_t slot_cfg = I2S_STD_MSB_SLOT_DEFAULT_CONFIG(I2S_DATA_BIT_WIDTH_16BIT, slot_mode);
        i2s_std_config_t std_cfg = {
          .clk_cfg = I2S_STD_CLK_DEFAULT_CONFIG(_currentSource.sampleRate),
          .slot_cfg = slot_cfg,
          .gpio_cfg = {
            .mclk = I2S_GPIO_UNUSED,
            .bclk = (gpio_num_t)config.pinAudioBCLK,
            .ws = (gpio_num_t)config.pinAudioLRC,
            .dout = (gpio_num_t)config.pinAudioDIN,
            .din = I2S_GPIO_UNUSED,
            .invert_flags = {.mclk_inv = false, .bclk_inv = false, .ws_inv = false},
          },
        };
        i2s_channel_init_std_mode(local_tx_handle, &std_cfg);
        i2s_channel_enable(local_tx_handle);

        int16_t sample_buffer[128];
        while (file.available() && _isPlaying) {
          size_t bytes_to_read = 64 * _currentSource.channels * sizeof(int16_t);
          size_t bytes_read = file.read((uint8_t*)sample_buffer, bytes_to_read);
          if (bytes_read == 0) break;
          applyVolume(sample_buffer, bytes_read / sizeof(int16_t), config.audio.volume);
          i2s_channel_write(local_tx_handle, sample_buffer, bytes_read, &bytes_written, portMAX_DELAY);
        }
        
        i2s_channel_disable(local_tx_handle);
        i2s_del_channel(local_tx_handle);
      }
      file.close();
    }
  } else {
    // Memory playback
    i2s_chan_config_t chan_cfg = I2S_CHANNEL_DEFAULT_CONFIG(I2S_NUM_0, I2S_ROLE_MASTER);
    if (i2s_new_channel(&chan_cfg, &local_tx_handle, NULL) == ESP_OK) {
      i2s_slot_mode_t slot_mode = (_currentSource.channels == 2) ? I2S_SLOT_MODE_STEREO : I2S_SLOT_MODE_MONO;
      i2s_std_slot_config_t slot_cfg = I2S_STD_MSB_SLOT_DEFAULT_CONFIG(I2S_DATA_BIT_WIDTH_16BIT, slot_mode);
      i2s_std_config_t std_cfg = {
        .clk_cfg = I2S_STD_CLK_DEFAULT_CONFIG(_currentSource.sampleRate),
        .slot_cfg = slot_cfg,
        .gpio_cfg = {
          .mclk = I2S_GPIO_UNUSED,
          .bclk = (gpio_num_t)config.pinAudioBCLK,
          .ws = (gpio_num_t)config.pinAudioLRC,
          .dout = (gpio_num_t)config.pinAudioDIN,
          .din = I2S_GPIO_UNUSED,
          .invert_flags = {.mclk_inv = false, .bclk_inv = false, .ws_inv = false},
        },
      };
      i2s_channel_init_std_mode(local_tx_handle, &std_cfg);
      i2s_channel_enable(local_tx_handle);

      int16_t sample_buffer[128];
      const int16_t* ptr = (const int16_t*)_currentSource.data;
      uint32_t remaining_samples = _currentSource.size / (_currentSource.channels * sizeof(int16_t));
      while (remaining_samples > 0 && _isPlaying) {
        size_t samples_to_read = remaining_samples > 64 ? 64 : remaining_samples;
        size_t bytes_to_read = samples_to_read * _currentSource.channels * sizeof(int16_t);
        memcpy(sample_buffer, ptr, bytes_to_read);
        applyVolume(sample_buffer, samples_to_read * _currentSource.channels, config.audio.volume);
        i2s_channel_write(local_tx_handle, sample_buffer, bytes_to_read, &bytes_written, portMAX_DELAY);
        ptr += (bytes_written / sizeof(int16_t));
        remaining_samples -= (bytes_written / (_currentSource.channels * sizeof(int16_t)));
      }
      
      i2s_channel_disable(local_tx_handle);
      i2s_del_channel(local_tx_handle);
    }
  }

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
