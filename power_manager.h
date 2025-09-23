#ifndef POWER_MANAGER_H
#define POWER_MANAGER_H

#include "esp_sleep.h"
#include "driver/rtc_io.h"   // Needed for RTC GPIO functions
#include "config.h"

class PowerManager {
private:
    unsigned long lastActivityTime;
    bool sleepEnabled;
    
    void setupWakeupSources();
    void enterDeepSleep();
    
public:
    PowerManager();
    void init();
    void updateActivity();
    void checkSleepConditions(bool isAudioPlaying);
    void enableSleep(bool enable);
    bool isSleepEnabled() const { return sleepEnabled; }
    unsigned long getTimeSinceActivity() const;
    void handleWakeup();
};

// Implementation
PowerManager::PowerManager() {
    lastActivityTime = millis();
    sleepEnabled = true;
}

void PowerManager::init() {
    // Print wake up reason
    handleWakeup();
    
    // Setup wake up sources
    setupWakeupSources();
    
    Serial.printf("Power management initialized. Sleep timeout: %lu seconds\n", SLEEP_TIMEOUT_MS / 1000);
}

void PowerManager::handleWakeup() {
    esp_sleep_wakeup_cause_t wakeup_reason = esp_sleep_get_wakeup_cause();
    
    switch(wakeup_reason) {
        case ESP_SLEEP_WAKEUP_EXT0:
            Serial.println("Wakeup caused by external signal using RTC_IO");
            break;

        case ESP_SLEEP_WAKEUP_EXT1: {
            Serial.println("Wakeup caused by external signal using RTC_CNTL");
            uint64_t wakeup_pin_mask = esp_sleep_get_ext1_wakeup_status();
            if (wakeup_pin_mask != 0) {
                for (int i = 0; i < NUM_BUTTONS; i++) {
                    int pin = BUTTON_PINS[i];
                    if (wakeup_pin_mask & (1ULL << pin)) {
                        Serial.printf("Wake up from Button %d (GPIO %d)\n", i + 1, pin);
                    }
                }
            }
            break;
        }

        case ESP_SLEEP_WAKEUP_TIMER:
            Serial.println("Wakeup caused by timer");
            break;

        case ESP_SLEEP_WAKEUP_TOUCHPAD:
            Serial.println("Wakeup caused by touchpad");
            break;

        case ESP_SLEEP_WAKEUP_ULP:
            Serial.println("Wakeup caused by ULP program");
            break;

        default:
            Serial.printf("Wakeup was not caused by deep sleep: %d\n", wakeup_reason);
            break;
    }
}

void PowerManager::setupWakeupSources() {
    uint64_t ext_wakeup_pin_mask = 0;
    
    for (int i = 0; i < NUM_BUTTONS; i++) {
        int pin = BUTTON_PINS[i];
        
        // Check if pin is RTC GPIO capable
        if (rtc_gpio_is_valid_gpio((gpio_num_t)pin)) {
            ext_wakeup_pin_mask |= (1ULL << pin);
            Serial.printf("Button %d (GPIO %d) configured as wake source\n", i + 1, pin);

            // Configure pin for wakeup: disable pull-up, enable pulldown
            rtc_gpio_pullup_dis((gpio_num_t)pin);
            rtc_gpio_pulldown_en((gpio_num_t)pin);
        } else {
            Serial.printf("Warning: GPIO %d is not RTC capable and cannot wake from deep sleep\n", pin);
        }
    }
    
    if (ext_wakeup_pin_mask > 0) {
        // Wake when ANY pin goes HIGH (our pulldown trick makes button press = HIGH)
        esp_sleep_enable_ext1_wakeup(ext_wakeup_pin_mask, ESP_EXT1_WAKEUP_ANY_HIGH);
        Serial.printf("Configured ext1 wakeup (ANY button press) with mask: 0x%llx\n", ext_wakeup_pin_mask);
    } else {
        Serial.println("Warning: No valid RTC GPIO pins found for wake up!");
    }

    // Optional: timer wake up as backup
    // esp_sleep_enable_timer_wakeup(SLEEP_TIMEOUT_MS * 1000); // Convert to microseconds
}

void PowerManager::updateActivity() {
    lastActivityTime = millis();
}

void PowerManager::checkSleepConditions(bool isAudioPlaying) {
    if (!sleepEnabled) {
        return;
    }
    
    // Don't sleep while audio is playing
    if (isAudioPlaying) {
        updateActivity(); // Reset activity timer while audio is playing
        return;
    }
    
    unsigned long timeSinceActivity = getTimeSinceActivity();
    
    // Check if it's time to sleep
    if (timeSinceActivity >= SLEEP_TIMEOUT_MS) {
        Serial.printf("Entering deep sleep after %lu seconds of inactivity\n", timeSinceActivity / 1000);
        
        // Give some time for serial output
        delay(100);
        
        enterDeepSleep();
    } else if (timeSinceActivity >= (SLEEP_TIMEOUT_MS - SLEEP_WARNING_TIME_MS)) {
        // Optional: Print warning before sleep (only once)
        static bool warningPrinted = false;
        if (!warningPrinted) {
            unsigned long timeLeft = (SLEEP_TIMEOUT_MS - timeSinceActivity) / 1000;
            Serial.printf("Warning: Will enter deep sleep in %lu seconds\n", timeLeft);
            warningPrinted = true;
        }
    }
}

void PowerManager::enterDeepSleep() {
    Serial.println("Preparing for deep sleep...");
    Serial.flush();
    
    // Disable WiFi to save power
    WiFi.disconnect(true);
    WiFi.mode(WIFI_OFF);
    
    Serial.println("Entering deep sleep now...");
    Serial.flush();
    
    esp_deep_sleep_start();
}

void PowerManager::enableSleep(bool enable) {
    sleepEnabled = enable;
    if (enable) {
        updateActivity(); // Reset timer when re-enabling
        Serial.println("Deep sleep enabled");
    } else {
        Serial.println("Deep sleep disabled");
    }
}

unsigned long PowerManager::getTimeSinceActivity() const {
    return millis() - lastActivityTime;
}

#endif
