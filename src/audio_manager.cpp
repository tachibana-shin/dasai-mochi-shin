#include "audio_manager.h"
#include <Arduino.h>
#include "config.h"
#include "filesystem.h"
#include "assets/audio/click.h"
#include "assets/audio/lockscreen.h"

#include <AudioFileSourceSD.h>
#include <AudioFileSourceSPIFFS.h>
#include <AudioFileSourcePROGMEM.h>
#include <AudioGeneratorWAV.h>
#include <AudioOutputI2S.h>

// Global pointers for ESP8266Audio
AudioGeneratorWAV *wav = nullptr;
AudioFileSourceSD *fileSD = nullptr;
AudioFileSourceSPIFFS *fileSPIFFS = nullptr;
AudioFileSourcePROGMEM *filePROGMEM = nullptr;
AudioOutputI2S *out = nullptr;

bool isPlaying = false;

void stopPlayback() {
    if (wav && wav->isRunning()) {
        wav->stop();
    }
    if (fileSD) {
        fileSD->close();
        delete fileSD;
        fileSD = nullptr;
    }
    if (fileSPIFFS) {
        fileSPIFFS->close();
        delete fileSPIFFS;
        fileSPIFFS = nullptr;
    }
    if (filePROGMEM) {
        filePROGMEM->close();
        delete filePROGMEM;
        filePROGMEM = nullptr;
    }
    isPlaying = false;
}

void initAudio() {
    out = new AudioOutputI2S();
    // Configure I2S pins from config
    out->SetPinout(config.pinAudioBCLK, config.pinAudioLRC, config.pinAudioDIN);
    out->SetGain((float)config.audio.volume / 20.0f);
    
    wav = new AudioGeneratorWAV();
}

void loopAudio() {
    if (wav && wav->isRunning()) {
        if (!wav->loop()) {
            stopPlayback();
        }
    }
}

void playAudio(const char* filename) {
    stopPlayback();

    String path = config.homePath + "/Audio/" + filename;
    
    // Check if file exists on SD or SPIFFS
    // Note: AudioFileSourceSD might need specific SPI class for multiple SD cards, 
    // but default SD.h usually works with AudioFileSourceSD.
    if (SD.exists(path)) {
        fileSD = new AudioFileSourceSD(path.c_str());
        if (wav->begin(fileSD, out)) {
            isPlaying = true;
            return;
        }
    } else if (SPIFFS.exists(path)) {
        fileSPIFFS = new AudioFileSourceSPIFFS(path.c_str());
        if (wav->begin(fileSPIFFS, out)) {
            isPlaying = true;
            return;
        }
    }
    
    Serial.println("Failed to play audio: " + String(filename));
}

void playAlarmAudio() {
    if (config.audio.alarmEnabled) playAudio("alarm.wav");
}

void playDrinkAudio() {
    if (config.audio.drinkEnabled) playAudio("drink.wav");
}

void playNotifyAudio() {
    if (config.audio.notifyEnabled) playAudio("notification.wav");
}

void playTapAudio() {
    // Check if enabled
    if (!config.audio.notifyEnabled) return;
    
    stopPlayback();

    const uint8_t* data = f__Users_Admin_dasai_mochi_shin_src_assets_audio_click_wav;
    size_t len = f__Users_Admin_dasai_mochi_shin_src_assets_audio_click_wav_len;

    filePROGMEM = new AudioFileSourcePROGMEM(data, len);
    if (wav->begin(filePROGMEM, out)) {
        isPlaying = true;
    } else {
        Serial.println("Failed to play click audio from PROGMEM");
    }
}

void playLockscreenAudio() {
    // Check if enabled (using notifyEnabled for now as a general system sound toggle)
    if (!config.audio.notifyEnabled) return;
    
    stopPlayback();

    const uint8_t* data = f__Users_Admin_dasai_mochi_shin_src_assets_audio_lockscreen_wav;
    size_t len = f__Users_Admin_dasai_mochi_shin_src_assets_audio_lockscreen_wav_len;

    filePROGMEM = new AudioFileSourcePROGMEM(data, len);
    if (wav->begin(filePROGMEM, out)) {
        isPlaying = true;
    } else {
        Serial.println("Failed to play lockscreen audio from PROGMEM");
    }
}

void setVolume(uint8_t volume) {
    config.audio.volume = volume;
    if (out) {
        out->SetGain((float)volume / 20.0f);
    }
}
