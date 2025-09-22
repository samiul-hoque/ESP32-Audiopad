#ifndef AUDIO_MANAGER_H
#define AUDIO_MANAGER_H

#include "AudioFileSourceSPIFFS.h"
#include "AudioFileSourceID3.h"
#include "AudioGeneratorMP3.h"
#include "AudioOutputI2S.h"
#include "config.h"

class AudioManager {
private:
    AudioGeneratorMP3 *mp3;
    AudioFileSourceSPIFFS *file;
    AudioOutputI2S *out;
    AudioFileSourceID3 *id3;
    bool isPlaying;
    float currentVolume;
    
public:
    AudioManager();
    ~AudioManager();
    void init();
    void playButtonSound(int buttonNum);
    void stopCurrentAudio();
    void update();
    void setVolume(float volume);
    float getVolume() const { return currentVolume; }
    bool getIsPlaying() const { return isPlaying; }
};

// Implementation
AudioManager::AudioManager() {
    mp3 = nullptr;
    file = nullptr;
    out = nullptr;
    id3 = nullptr;
    isPlaying = false;
    currentVolume = DEFAULT_AUDIO_GAIN;
}

AudioManager::~AudioManager() {
    stopCurrentAudio();
    if (out) {
        delete out;
        out = nullptr;
    }
}

void AudioManager::init() {
    // Initialize audio output
    out = new AudioOutputI2S();
    out->SetPinout(I2S_BCLK_PIN, I2S_LRC_PIN, I2S_DIN_PIN); // BCLK, LRC, DIN
    out->SetGain(currentVolume); // Use current volume setting
}

void AudioManager::setVolume(float volume) {
    // Clamp volume to valid range
    currentVolume = constrain(volume, MIN_AUDIO_GAIN, MAX_AUDIO_GAIN);
    
    // Apply to audio output if it exists
    if (out) {
        out->SetGain(currentVolume);
        Serial.printf("Volume set to: %.2f\n", currentVolume);
    }
}

void AudioManager::update() {
    // Handle MP3 playback
    if (mp3 && mp3->isRunning()) {
        if (!mp3->loop()) {
            mp3->stop();
            delete file;
            delete id3;
            delete mp3;
            mp3 = nullptr;
            file = nullptr;
            id3 = nullptr;
            isPlaying = false;
            Serial.println("MP3 playback finished");
        }
    }
}

void AudioManager::stopCurrentAudio() {
    if (mp3 && mp3->isRunning()) {
        mp3->stop();
        delete file;
        delete id3;
        delete mp3;
        mp3 = nullptr;
        file = nullptr;
        id3 = nullptr;
        isPlaying = false;
        Serial.println("Audio stopped by request");
    }
}

void AudioManager::playButtonSound(int buttonNum) {
    Serial.printf("playButtonSound called for button %d\n", buttonNum);
    
    if (isPlaying) {
        Serial.println("Stopping current audio...");
        stopCurrentAudio();
    }
    
    String filename = "/audio/button" + String(buttonNum) + ".mp3";
    Serial.printf("Looking for file: %s\n", filename.c_str());
    
    if (!SPIFFS.exists(filename)) {
        Serial.printf("File %s not found\n", filename.c_str());
        return;
    }
    
    Serial.printf("File %s exists, starting playback...\n", filename.c_str());
    
    file = new AudioFileSourceSPIFFS(filename.c_str());
    if (!file) {
        Serial.println("Failed to create AudioFileSourceSPIFFS");
        return;
    }
    
    id3 = new AudioFileSourceID3(file);
    if (!id3) {
        Serial.println("Failed to create AudioFileSourceID3");
        delete file;
        file = nullptr;
        return;
    }
    
    mp3 = new AudioGeneratorMP3();
    if (!mp3) {
        Serial.println("Failed to create AudioGeneratorMP3");
        delete file;
        delete id3;
        file = nullptr;
        id3 = nullptr;
        return;
    }
    
    Serial.println("Starting MP3 playback...");
    isPlaying = true;
    
    if (!mp3->begin(id3, out)) {
        Serial.println("Error starting MP3 decoder");
        isPlaying = false;
        delete file;
        delete id3;
        delete mp3;
        file = nullptr;
        id3 = nullptr;
        mp3 = nullptr;
    } else {
        Serial.println("MP3 playback started successfully");
    }
}

#endif