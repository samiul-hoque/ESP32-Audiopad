#include <WiFi.h>
#include <SPIFFS.h>
#include "secrets.h"

// Include all our custom headers
#include "config.h"
#include "button_manager.h"
#include "audio_manager.h"
#include "web_server.h"
#include "ota_manager.h"
#include "power_manager.h"

// Create instances of our managers
ButtonManager buttonManager;
AudioManager audioManager;
WebServerManager webServer;
OTAManager otaManager;
PowerManager powerManager;

// Timing variables for power management
unsigned long lastActivityCheck = 0;

// Callback functions
void onButtonPressed(int buttonNum) {
    powerManager.updateActivity(); // Update activity on button press
    audioManager.playButtonSound(buttonNum);
}

void onTestButtonPressed(int buttonNum) {
    powerManager.updateActivity(); // Update activity on web test
    audioManager.playButtonSound(buttonNum);
}

void onStopAudio() {
    powerManager.updateActivity(); // Update activity on stop command
    audioManager.stopCurrentAudio();
}

void onSetVolume(float volume) {
    powerManager.updateActivity(); // Update activity on volume change
    audioManager.setVolume(volume);
}

float onGetVolume() {
    powerManager.updateActivity(); // Update activity on volume request
    return audioManager.getVolume();
}

void setup() {
    Serial.begin(115200);
    
    // Initialize power management first (handles wake up reasons)
    powerManager.init();
    
    // Initialize SPIFFS
    if(!SPIFFS.begin(true)) {
        Serial.println("SPIFFS Mount Failed");
        return;
    }
    
    // Create audio directory if it doesn't exist
    if (!SPIFFS.exists("/audio")) {
        SPIFFS.mkdir("/audio");
    }

    // Print SPIFFS info
    size_t totalBytes = SPIFFS.totalBytes();
    size_t usedBytes = SPIFFS.usedBytes();
    Serial.printf("SPIFFS Total: %d bytes, Used: %d bytes, Free: %d bytes\n", 
                  totalBytes, usedBytes, totalBytes - usedBytes);
    
    // Initialize battery pin
    pinMode(BATTERY_PIN, INPUT);
    
    // Initialize all managers
    buttonManager.init();
    audioManager.init();
    
    // Set up button callback
    buttonManager.onButtonPressed = onButtonPressed;
    
    // Connect to WiFi
    WiFi.begin(ssid, password);
    while (WiFi.status() != WL_CONNECTED) {
        delay(1000);
        Serial.println("Connecting to WiFi...");
    }
    Serial.println("WiFi connected!");
    Serial.print("IP address: ");
    Serial.println(WiFi.localIP());
    
    // Update activity after WiFi connection
    powerManager.updateActivity();
    
    // Initialize OTA and web server
    otaManager.init();
    webServer.init();
    
    // Set up web server callbacks
    webServer.setTestButtonCallback(onTestButtonPressed);
    webServer.setStopAudioCallback(onStopAudio);
    webServer.setVolumeCallbacks(onSetVolume, onGetVolume);
    
    Serial.println("System initialized successfully!");
    Serial.printf("Deep sleep will activate after %lu seconds of inactivity\n", SLEEP_TIMEOUT_MS / 1000);
}

void loop() {
    // Handle all the different components
    otaManager.handle();
    
    // Update activity on any web server interaction
    static bool hadWebActivity = false;
    webServer.handleClient();
    // Note: We could implement a more sophisticated web activity detection
    // by modifying the web server to report when it handles requests
    
    audioManager.update();
    buttonManager.checkButtons();
    
    // Periodically check sleep conditions
    unsigned long currentTime = millis();
    if (currentTime - lastActivityCheck >= ACTIVITY_UPDATE_INTERVAL) {
        lastActivityCheck = currentTime;
        
        // Check if we should enter deep sleep
        powerManager.checkSleepConditions(audioManager.getIsPlaying());
        
        // Optional: Print activity status for debugging
        if (powerManager.getTimeSinceActivity() > 60000) { // Only after 1 minute
            unsigned long timeLeft = (SLEEP_TIMEOUT_MS - powerManager.getTimeSinceActivity()) / 1000;
            if (timeLeft < 60) { // Only print when close to sleep
                Serial.printf("Time until sleep: %lu seconds\n", timeLeft);
            }
        }
    }
    
    delay(10);
}