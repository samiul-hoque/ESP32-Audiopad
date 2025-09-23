#ifndef WEB_SERVER_H
#define WEB_SERVER_H

#include <WebServer.h>
#include <SPIFFS.h>
#include <FS.h>
#include "web_interface.h"
#include "config.h"

class WebServerManager {
private:
    WebServer* server;
    File uploadFile;
    String uploadFilename;
    
    // Function pointers for callbacks
    void (*onTestButton)(int buttonNum) = nullptr;
    void (*onStopAudio)() = nullptr;
    void (*onSetVolume)(float volume) = nullptr;
    float (*onGetVolume)() = nullptr;
    void (*onWebActivity)() = nullptr; // New callback for web activity
    
    // Helper to update activity for all requests
    void updateWebActivity();
    
public:
    WebServerManager();
    ~WebServerManager();
    void init();
    void handleClient();
    void setTestButtonCallback(void (*callback)(int));
    void setStopAudioCallback(void (*callback)());
    void setVolumeCallbacks(void (*setCallback)(float), float (*getCallback)());
    void setWebActivityCallback(void (*callback)()); // New method
    
    // Handler functions
    void handleRoot();
    void handleCSS();
    void handleBattery();
    void handleFileUpload();
    void handleUploadResult();
    void handleListFiles();
    void handleDeleteFile();
    void handleTestButton();
    void handleStopAudio();
    void handleSetVolume();
    void handleGetVolume();
};

// Implementation
WebServerManager::WebServerManager() {
    server = new WebServer(80);
}

WebServerManager::~WebServerManager() {
    if (server) {
        delete server;
        server = nullptr;
    }
}

void WebServerManager::updateWebActivity() {
    if (onWebActivity != nullptr) {
        onWebActivity();
    }
}

void WebServerManager::init() {
    // Setup web server routes
    server->on("/", HTTP_GET, [this](){ this->handleRoot(); });
    server->on("/battery", HTTP_GET, [this](){ this->handleBattery(); }); 
    server->on("/upload", HTTP_POST, [this](){ this->handleUploadResult(); }, [this](){ this->handleFileUpload(); });
    server->on("/files", HTTP_GET, [this](){ this->handleListFiles(); });
    server->on("/delete", HTTP_POST, [this](){ this->handleDeleteFile(); });
    server->on("/test", HTTP_POST, [this](){ this->handleTestButton(); });
    server->on("/stop", HTTP_POST, [this](){ this->handleStopAudio(); });
    server->on("/volume", HTTP_POST, [this](){ this->handleSetVolume(); });
    server->on("/volume", HTTP_GET, [this](){ this->handleGetVolume(); });
    server->on("/style.css", HTTP_GET, [this](){ this->handleCSS(); });
    
    server->begin();
    Serial.println("HTTP server started");
}

void WebServerManager::handleClient() {
    server->handleClient();
}

void WebServerManager::setTestButtonCallback(void (*callback)(int)) {
    onTestButton = callback;
}

void WebServerManager::setStopAudioCallback(void (*callback)()) {
    onStopAudio = callback;
}

void WebServerManager::setVolumeCallbacks(void (*setCallback)(float), float (*getCallback)()) {
    onSetVolume = setCallback;
    onGetVolume = getCallback;
}

void WebServerManager::setWebActivityCallback(void (*callback)()) {
    onWebActivity = callback;
}

void WebServerManager::handleCSS() {
    updateWebActivity();
    server->send(200, "text/css", WEB_CSS);
}

void WebServerManager::handleRoot() {
    updateWebActivity();
    server->send(200, "text/html", WEB_HTML);
}

void WebServerManager::handleTestButton() {
    updateWebActivity();
    if (server->hasArg("button")) {
        int buttonNum = server->arg("button").toInt();
        if (buttonNum >= 1 && buttonNum <= 6) {
            if (onTestButton != nullptr) {
                onTestButton(buttonNum);
            }
            server->send(200, "text/plain", "Testing button " + String(buttonNum));
        } else {
            server->send(400, "text/plain", "Invalid button number");
        }
    } else {
        server->send(400, "text/plain", "Missing button parameter");
    }
}

void WebServerManager::handleStopAudio() {
    updateWebActivity();
    if (onStopAudio != nullptr) {
        onStopAudio();
    }
    server->send(200, "text/plain", "Audio stopped");
}

void WebServerManager::handleBattery() {
    updateWebActivity();
    int adcValue = analogRead(BATTERY_PIN);
    float voltage = adcValue * ADC_TO_VOLT;
    String json = "{\"voltage\": " + String(voltage) + "}";
    server->send(200, "application/json", json);
}

void WebServerManager::handleFileUpload() {
    updateWebActivity();
    HTTPUpload& upload = server->upload();
    if (upload.status == UPLOAD_FILE_START) {
        if (!server->hasArg("button")) {
            Serial.println("Upload started without button number!");
            return;
        }
        uploadFilename = "/audio/button" + server->arg("button") + ".mp3";
        
        if (SPIFFS.exists(uploadFilename)) {
            SPIFFS.remove(uploadFilename);
        }
        
        uploadFile = SPIFFS.open(uploadFilename, "w");
        if (!uploadFile) {
            Serial.printf("Failed to create file: %s\n", uploadFilename.c_str());
            return;
        }
        Serial.printf("Upload Start: %s\n", uploadFilename.c_str());
    } else if (upload.status == UPLOAD_FILE_WRITE) {
        if (uploadFile) {
            size_t bytesWritten = uploadFile.write(upload.buf, upload.currentSize);
            if (bytesWritten != upload.currentSize) {
                Serial.println("File write failed");
                uploadFile.close();
            }
        }
    } else if (upload.status == UPLOAD_FILE_END) {
        if (uploadFile) {
            uploadFile.close();
            Serial.printf("Upload End: %s, %u bytes\n", uploadFilename.c_str(), upload.totalSize);
        } else {
            Serial.println("Upload file not open.");
        }
    }
}

void WebServerManager::handleUploadResult() {
    updateWebActivity();
    // The file upload is complete. Now we can safely report success.
    String json = "{\"status\":\"success\", \"message\":\"File uploaded successfully!\"}";
    server->send(200, "application/json", json);
    delay(100); 
}

void WebServerManager::handleListFiles() {
    updateWebActivity();
    Serial.println("Listing files in /audio directory:");
    
    String json = "{\"files\":[";
    bool first = true;
    
    // Check each specific button file
    for (int i = 1; i <= NUM_BUTTONS; i++) {
        String buttonFile = "/audio/button" + String(i) + ".mp3";
        if (SPIFFS.exists(buttonFile)) {
            if (!first) {
                json += ",";
            }
            json += "\"button" + String(i) + ".mp3\"";
            first = false;
            Serial.println("Found: " + buttonFile);
        }
    }
    
    json += "]}";
    Serial.println("JSON response: " + json);
    server->send(200, "application/json", json); 
}

void WebServerManager::handleDeleteFile() {
    updateWebActivity();
    if (server->hasArg("filename")) {
        String filename = "/audio/" + server->arg("filename");
        if (SPIFFS.exists(filename)) {
            SPIFFS.remove(filename);
            server->send(200, "text/plain", "File deleted");
            Serial.println("Deleted file: " + filename);
        } else {
            server->send(404, "text/plain", "File not found");
        }
    } else {
        server->send(400, "text/plain", "Missing filename");
    }
}

void WebServerManager::handleSetVolume() {
    updateWebActivity();
    if (server->hasArg("volume")) {
        float volume = server->arg("volume").toFloat();
        if (onSetVolume != nullptr) {
            onSetVolume(volume);
        }
        server->send(200, "text/plain", "Volume set to " + String(volume));
    } else {
        server->send(400, "text/plain", "Missing volume parameter");
    }
}

void WebServerManager::handleGetVolume() {
    updateWebActivity();
    float volume = 0.5; // Default value
    if (onGetVolume != nullptr) {
        volume = onGetVolume();
    }
    String json = "{\"volume\": " + String(volume) + "}";
    server->send(200, "application/json", json);
}

#endif