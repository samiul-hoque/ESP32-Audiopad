#ifndef CONFIG_H
#define CONFIG_H

// Hardware pin definitions
const int BUTTON_PINS[] = {13, 14, 27, 26, 25, 32};
const int BATTERY_PIN = 35;

// Battery voltage conversion
const float ADC_TO_VOLT = 7.285f / 4096.0f;    

// File size limit (in bytes)
const size_t MAX_FILE_SIZE = 500000; // 500KB per file

// Button debouncing
const unsigned long DEBOUNCE_DELAY = 50;

// Audio gain settings (0.0 to 1.0)
const float DEFAULT_AUDIO_GAIN = 0.5;
const float MIN_AUDIO_GAIN = 0.0;
const float MAX_AUDIO_GAIN = 1.0;

// I2S audio pins
const int I2S_BCLK_PIN = 4;
const int I2S_LRC_PIN = 2;
const int I2S_DIN_PIN = 15;

// Battery update interval (milliseconds)
const unsigned long BATTERY_UPDATE_INTERVAL = 10000;

// Number of buttons
const int NUM_BUTTONS = 6;

// Power management settings
const unsigned long SLEEP_TIMEOUT_MS = 300000;        // 5 minutes (300,000ms) - configurable sleep timeout
const unsigned long SLEEP_WARNING_TIME_MS = 30000;    // 30 seconds warning before sleep
const unsigned long ACTIVITY_UPDATE_INTERVAL = 5000;  // Check activity every 5 seconds

#endif