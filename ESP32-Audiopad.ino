#include <WiFi.h>
#include <SPIFFS.h>
#include "secrets.h"

// Include all our custom headers
#include "config.h"
#include "button_manager.h"
#include "audio_manager.h"
#include "web_server.h"
#include "ota_manager.h"

// Create instances of our managers
ButtonManager buttonManager;
AudioManager audioManager;
WebServerManager webServer;
OTAManager otaManager;

// Callback functions
void onButtonPressed(int buttonNum) {
    audioManager.playButtonSound(buttonNum);
}

void onTestButtonPressed(int buttonNum) {
    audioManager.playButtonSound(buttonNum);
}

void onStopAudio() {
    audioManager.stopCurrentAudio();
}

void onSetVolume(float volume) {
    audioManager.setVolume(volume);
}

float onGetVolume() {
    return audioManager.getVolume();
}

void setup() {
    Serial.begin(115200);
    
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
    
    // Initialize OTA and web server
    otaManager.init();
    webServer.init();
    
    // Set up web server callbacks
    webServer.setTestButtonCallback(onTestButtonPressed);
    webServer.setStopAudioCallback(onStopAudio);
    webServer.setVolumeCallbacks(onSetVolume, onGetVolume);
    
    Serial.println("System initialized successfully!");
}

void loop() {
    // Handle all the different components
    otaManager.handle();
    webServer.handleClient();
    audioManager.update();
    buttonManager.checkButtons();
    
    delay(10);
}