#include <WiFi.h>
#include <WebServer.h>
#include <ArduinoOTA.h>
#include <SPIFFS.h>
#include <FS.h>
#include <Update.h>
#include "secrets.h"

// Use a different audio library that's more compatible
#include "AudioFileSourceSPIFFS.h"
#include "AudioFileSourceID3.h"
#include "AudioGeneratorMP3.h"
#include "AudioOutputI2S.h"


// Hardware pin definitions
const int BUTTON_PINS[] = {25, 26, 27, 13, 14, 32};
const int BATTERY_PIN = 35;

// Audio objects
AudioGeneratorMP3 *mp3;
AudioFileSourceSPIFFS *file;
AudioOutputI2S *out;
AudioFileSourceID3 *id3;

// Web server
WebServer server(80);

// Button state tracking
bool lastButtonState[6] = {HIGH, HIGH, HIGH, HIGH, HIGH, HIGH};
bool currentButtonState[6] = {HIGH, HIGH, HIGH, HIGH, HIGH, HIGH};
unsigned long lastDebounceTime[6] = {0, 0, 0, 0, 0, 0};
const unsigned long debounceDelay = 50;

// Battery voltage conversion
const float ADC_TO_VOLT = 7.285f / 4096.0f;    

// File size limit (in bytes)
const size_t MAX_FILE_SIZE = 500000; // 500KB per file

// Audio state
bool isPlaying = false;

// Function Prototypes
void setupOTA();
void setupWebServer();
void handleRoot();
void handleCSS();
void handleBattery();
void handleFileUpload();
void handleUploadResult();
void handleListFiles();
void handleDeleteFile();
void handleTestButton();
void handleStopAudio();
void checkButtons();
void stopCurrentAudio();
void playButtonSound(int buttonNum);

void setup() {
  Serial.begin(115200);
  
  // Initialize buttons (using external pull-up resistors)
  for(int i = 0; i < 6; i++) {
    pinMode(BUTTON_PINS[i], INPUT);  // Changed from INPUT_PULLUP to INPUT
  }
  
  // Initialize battery pin
  pinMode(BATTERY_PIN, INPUT);
  
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
  
  // Initialize audio output
  out = new AudioOutputI2S();
  out->SetPinout(4, 2, 15); // BCLK, LRC, DIN
  out->SetGain(0.5); // Adjust volume (0.0 to 1.0) 
  
  // Connect to WiFi
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Connecting to WiFi...");
  }
  Serial.println("WiFi connected!");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
  
  // Setup OTA
  setupOTA();
  
  // Setup web server routes
  setupWebServer();
  
  server.begin();
  Serial.println("HTTP server started");
}

void loop() {
  ArduinoOTA.handle();
  server.handleClient();
  
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
  
  // Check buttons
  checkButtons();
  
  delay(10);
}

void setupOTA() {
  ArduinoOTA.setHostname("ESP32-AudioController");
  
  ArduinoOTA.onStart([]() {
    String type;
    if (ArduinoOTA.getCommand() == U_FLASH) {
      type = "sketch";
    } else { // U_SPIFFS
      type = "filesystem";
    }
    Serial.println("Start updating " + type);
  });
  
  ArduinoOTA.onEnd([]() {
    Serial.println("\nEnd");
  });
  
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
  });
  
  ArduinoOTA.onError([](ota_error_t error) {
    Serial.printf("Error[%u]: ", error);
    if (error == OTA_AUTH_ERROR) {
      Serial.println("Auth Failed");
    } else if (error == OTA_BEGIN_ERROR) {
      Serial.println("Begin Failed");
    } else if (error == OTA_CONNECT_ERROR) {
      Serial.println("Connect Failed");
    } else if (error == OTA_RECEIVE_ERROR) {
      Serial.println("Receive Failed");
    } else if (error == OTA_END_ERROR) {
      Serial.println("End Failed");
    }
  });
  
  ArduinoOTA.begin();
}

void setupWebServer() {
  // Main page
  server.on("/", HTTP_GET, handleRoot);
  
  // API endpoints
  server.on("/battery", HTTP_GET, handleBattery); 
  server.on("/upload", HTTP_POST, handleUploadResult, handleFileUpload); // Keep this for backward compatibility or direct calls
  server.on("/files", HTTP_GET, handleListFiles);
  server.on("/delete", HTTP_POST, handleDeleteFile);
  server.on("/test", HTTP_POST, handleTestButton);
  server.on("/stop", HTTP_POST, handleStopAudio);
  
  // CSS
  server.on("/style.css", HTTP_GET, handleCSS);
}

void handleCSS() {
  String css = R"=====(
body { font-family: sans-serif; background-color: #f4f4f4; color: #333; margin: 0; padding: 20px; }
.container { max-width: 800px; margin: auto; background: white; padding: 20px; border-radius: 8px; box-shadow: 0 2px 4px rgba(0,0,0,0.1); }
h1, h2 { color: #0056b3; }
.section { margin-bottom: 20px; border-bottom: 1px solid #eee; padding-bottom: 20px; }
.info { font-size: 0.9em; color: #666; }
#battery-info { display: flex; align-items: center; }
.battery-bar { width: 100px; height: 20px; border: 1px solid #ccc; border-radius: 4px; margin-left: 10px; background-color: #e9e9e9; }
#battery-level { height: 100%; border-radius: 3px; }
.battery-good { background-color: #4CAF50; }
.battery-medium { background-color: #ffc107; }
.battery-low { background-color: #f44336; }
.upload-grid { display: grid; grid-template-columns: 1fr; gap: 15px; }
@media (min-width: 600px) { .upload-grid { grid-template-columns: repeat(2, 1fr); } }
.upload-item { background: #f9f9f9; padding: 15px; border-radius: 5px; border: 1px solid #ddd; }
.upload-item label { font-weight: bold; display: block; margin-bottom: 5px; }
input[type="file"] { margin-bottom: 10px; }
button, .stop-btn { background-color: #007bff; color: white; border: none; padding: 10px 15px; border-radius: 4px; cursor: pointer; font-size: 1em; }
button:hover, .stop-btn:hover { background-color: #0056b3; }
.stop-btn { background-color: #dc3545; }
.stop-btn:hover { background-color: #c82333; }
.file-item { display: flex; justify-content: space-between; align-items: center; padding: 8px; border-bottom: 1px solid #eee; }
)=====";
  server.send(200, "text/css", css);
}

void handleRoot() {
  String html = R"=====( 
<!DOCTYPE html>
<html>
<head>
    <title>ESP32 Audio Controller</title>
    <link rel="stylesheet" href="/style.css">
    <meta name="viewport" content="width=device-width, initial-scale=1">
</head>
<body>
    <div class="container">
        <h1>ESP32 Audio Controller</h1>
        
        <div class="section">
            <h2>Battery Status</h2>
            <div id="battery-info">
                <span id="battery-voltage">Loading...</span>V
                <div class="battery-bar">
                    <div id="battery-level"></div>
                </div>
            </div>
        </div>
        
        <div class="section">
            <h2>Audio Control</h2>
            <button onclick="stopAudio()" class="stop-btn">Stop All Audio</button>
        </div>
        
        <div class="section">
            <h2>Audio Files</h2>
            <p class="info">Maximum file size: 500KB per file. Supported format: MP3<br>
            Recommended: 64kbps mono or 32kbps stereo for longer audio</p>
            <div id="file-list"></div>
        </div>
        
        <div class="section">
            <h2>Upload Audio Files</h2>
            <form id="upload-form" enctype="multipart/form-data">
                <div class="upload-grid">
                    <div class="upload-item">
                        <label>Button 1 (Pin 25):</label>
                        <input type="file" name="file" data-button="1" accept=".mp3">
                        <button type="button" onclick="uploadFile(1)">Upload</button>
                        <button type="button" onclick="testButton(1)">Test</button>
                    </div>
                    <div class="upload-item">
                        <label>Button 2 (Pin 26):</label>
                        <input type="file" name="file" data-button="2" accept=".mp3">
                        <button type="button" onclick="uploadFile(2)">Upload</button>
                        <button type="button" onclick="testButton(2)">Test</button>
                    </div>
                    <div class="upload-item">
                        <label>Button 3 (Pin 27):</label>
                        <input type="file" name="file" data-button="3" accept=".mp3">
                        <button type="button" onclick="uploadFile(3)">Upload</button>
                        <button type="button" onclick="testButton(3)">Test</button>
                    </div>
                    <div class="upload-item">
                        <label>Button 4 (Pin 13):</label>
                        <input type="file" name="file" data-button="4" accept=".mp3">
                        <button type="button" onclick="uploadFile(4)">Upload</button>
                        <button type="button" onclick="testButton(4)">Test</button>
                    </div>
                    <div class="upload-item">
                        <label>Button 5 (Pin 14):</label>
                        <input type="file" name="file" data-button="5" accept=".mp3">
                        <button type="button" onclick="uploadFile(5)">Upload</button>
                        <button type="button" onclick="testButton(5)">Test</button>
                    </div>
                    <div class="upload-item">
                        <label>Button 6 (Pin 32):</label>
                        <input type="file" name="file" data-button="6" accept=".mp3">
                        <button type="button" onclick="uploadFile(6)">Upload</button>
                        <button type="button" onclick="testButton(6)">Test</button>
                    </div>
                </div>
            </form>
        </div>
        
        <div id="status"></div>
    </div>

    <script>
        function updateBattery() {
            fetch('/battery')
                .then(response => response.json())
                .then(data => {
                    document.getElementById('battery-voltage').textContent = data.voltage.toFixed(2);
                    const percentage = Math.min(100, Math.max(0, (data.voltage - 3.0) / (4.2 - 3.0) * 100));
                    document.getElementById('battery-level').style.width = percentage + '%';
                    
                    // Color coding
                    const batteryLevel = document.getElementById('battery-level');
                    if (percentage > 50) {
                        batteryLevel.className = 'battery-good';
                    } else if (percentage > 20) {
                        batteryLevel.className = 'battery-medium';
                    } else {
                        batteryLevel.className = 'battery-low';
                    }
                })
                .catch(error => {
                    console.error('Error fetching battery status:', error);
                });
        }
        
        function updateFileList() {
          console.log('Updating file list...');

            fetch('/files')
                .then(response => response.json())
                .then(data => {
                    const fileList = document.getElementById('file-list');
                    fileList.innerHTML = '';
                    
                    console.log('Raw JSON response:', data);

                    for (let i = 1; i <= 6; i++) {
                        const filename = 'button' + i + '.mp3';
                        const exists = data.files && data.files.includes(filename);
                        const div = document.createElement('div');
                        div.className = 'file-item';
                        div.innerHTML = `
                            <span>Button ${i}: ${exists ? '[File Assigned] ' + filename : '[No File Assigned]'}</span>
                            ${exists ? `<button onclick="deleteFile('${filename}')">Delete</button>` : ''}
                        `;
                        fileList.appendChild(div);
                    }
                })
                .catch(error => {
                    console.error('Error updating file list:', error);
                });
        }
        
        function uploadFile(buttonNum) {
            const input = document.querySelector(`input[data-button="${buttonNum}"]`);
            const file = input.files[0];
            
            if (!file) {
                alert('Please select a file first');
                return;
            }
            
            if (file.size > 500000) {
                alert('File too large! Maximum size is 500KB');
                return;
            }
            
            const formData = new FormData();
            formData.append('file', file);
            formData.append('button', buttonNum);
            
            document.getElementById('status').innerHTML = `Uploading to button ${buttonNum}...`;
            
            fetch('/upload?button=' + buttonNum, {
                method: 'POST',
                body: formData
            }).then(response => {
              console.log('Upload response:', response);
              return response.json(); // Expect a JSON response now
            })
            .then(data => {
                document.getElementById('status').innerHTML = data.message;
                input.value = '';
                // Update file list after successful upload
                setTimeout(updateFileList, 200);
            })
            .catch(error => {
                document.getElementById('status').innerHTML = 'Upload failed: ' + error;
            });
        }
        
        function testButton(buttonNum) {
            fetch('/test', {
                method: 'POST',
                headers: {'Content-Type': 'application/x-www-form-urlencoded'},
                body: 'button=' + buttonNum
            })
            .then(response => response.text())
            .then(result => {
                document.getElementById('status').innerHTML = result;
            });
        }
        
        function stopAudio() {
            fetch('/stop', {
                method: 'POST'
            })
            .then(response => response.text())
            .then(result => {
                document.getElementById('status').innerHTML = result;
            });
        }
        
        function deleteFile(filename) {
            if (confirm('Are you sure you want to delete ' + filename + '?')) {
                fetch('/delete', {
                    method: 'POST', 
                    headers: {'Content-Type': 'application/x-www-form-urlencoded'},
                    body: 'filename=' + filename
                })
                .then(response => response.text())
                .then(result => { 
                    document.getElementById('status').innerHTML = result;
                    setTimeout(updateFileList, 200); // Give server a moment before refetching
                });
            }
        }
        
        // Initial load and periodic updates
        updateBattery();
        updateFileList();
        setInterval(updateBattery, 10000); // Update battery every 10 seconds
    </script>
</body>
</html>
)=====";
  server.send(200, "text/html", html);
}; 

void handleTestButton() {
  if (server.hasArg("button")) {
    int buttonNum = server.arg("button").toInt();
    if (buttonNum >= 1 && buttonNum <= 6) {
      playButtonSound(buttonNum);
      server.send(200, "text/plain", "Testing button " + String(buttonNum));
    } else {
      server.send(400, "text/plain", "Invalid button number");
    }
  } else {
    server.send(400, "text/plain", "Missing button parameter");
  }
}

void handleStopAudio() {
  stopCurrentAudio();
  server.send(200, "text/plain", "Audio stopped");
}

void handleBattery() {
  int adcValue = analogRead(BATTERY_PIN);
  float voltage = adcValue * ADC_TO_VOLT;
  String json = "{\"voltage\": " + String(voltage) + "}";
  server.send(200, "application/json", json);
}

File uploadFile;
String uploadFilename; // Add a global variable to store the filename

void handleFileUpload() {
  HTTPUpload& upload = server.upload();
  if (upload.status == UPLOAD_FILE_START) {
    if (!server.hasArg("button")) {
      Serial.println("Upload started without button number!");
      return;
    }
    uploadFilename = "/audio/button" + server.arg("button") + ".mp3";
    
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

void handleUploadResult() {
  // The file upload is complete. Now we can safely report success.
  // We will send a JSON response.
  String json = "{\"status\":\"success\", \"message\":\"File uploaded successfully!\"}";
  server.send(200, "application/json", json);

  // A small delay to ensure the filesystem is fully updated before any potential new requests.
  delay(100); 
}

void handleListFiles() {
  Serial.println("Listing files in /audio directory:");
  
  String json = "{\"files\":[";
  bool first = true;
  
  // Check each specific button file
  for (int i = 1; i <= 6; i++) {
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
  server.send(200, "application/json", json); 
}

void handleDeleteFile() {
  if (server.hasArg("filename")) {
    String filename = "/audio/" + server.arg("filename");
    if (SPIFFS.exists(filename)) {
      SPIFFS.remove(filename);
      server.send(200, "text/plain", "File deleted");
      Serial.println("Deleted file: " + filename);
    } else {
      server.send(404, "text/plain", "File not found");
    }
  } else {
    server.send(400, "text/plain", "Missing filename");
  }
}

void checkButtons() {
  // Check each button
  for (int i = 0; i < 6; i++) {
    // Read current button state
    bool reading = digitalRead(BUTTON_PINS[i]);
    
    // Check if button state has changed (for debouncing)
    if (reading != lastButtonState[i]) {
      lastDebounceTime[i] = millis(); // Reset debounce timer
    }
    
    // Check if enough time has passed since last state change
    if ((millis() - lastDebounceTime[i]) > debounceDelay) {
      // If the reading is different from current state
      if (reading != currentButtonState[i]) {
        currentButtonState[i] = reading;
        
        // Button pressed (LOW because button connects to ground)
        if (currentButtonState[i] == LOW) {
          Serial.print("✓ BUTTON ");
          Serial.print(i + 1);
          Serial.print(" PRESSED  -> GPIO ");
          Serial.print(BUTTON_PINS[i]);
          Serial.println(" = LOW");
          playButtonSound(i + 1);
        }
        // Button released (HIGH because of pull-up resistor)
        else {
          Serial.print("✗ BUTTON ");
          Serial.print(i + 1);
          Serial.print(" RELEASED -> GPIO ");
          Serial.print(BUTTON_PINS[i]);
          Serial.println(" = HIGH");
        }
      }
    }
    
    // Save the current reading for next loop iteration
    lastButtonState[i] = reading;
  }
}

void stopCurrentAudio() {
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

void playButtonSound(int buttonNum) {
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