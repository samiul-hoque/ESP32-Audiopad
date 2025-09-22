#ifndef WEB_INTERFACE_H
#define WEB_INTERFACE_H

// CSS styles for the web interface
const char* WEB_CSS = R"=====(
body { font-family: sans-serif; background-color: #f4f4f4; color: #333; margin: 0; padding: 10px; }
.container { max-width: 800px; margin: auto; background: white; padding: 10px; border-radius: 8px; box-shadow: 0 2px 4px rgba(0,0,0,0.1); }
h1, h2 { color: #0056b3; }
.section { margin-bottom: 15px; border: 1px solid #ddd; padding: 10px; border-radius: 5px; }
.top-row { display: flex; gap: 15px; flex-wrap: wrap; margin-bottom: 15px; }
.top-row .section { flex: 1; min-width: 250px; margin-bottom: 0; }
.info { font-size: 0.9em; color: #666; }
#battery-info { display: flex; align-items: center; }
.main-content { display: grid; grid-template-columns: 1fr; gap: 15px; }
@media (min-width: 768px) { .main-content { grid-template-columns: 1fr 1fr; } }
.battery-bar { width: 100px; height: 20px; border: 1px solid #ccc; border-radius: 4px; margin-left: 10px; background-color: #e9e9e9; }
#battery-level { height: 100%; border-radius: 3px; }
.battery-good { background-color: #4CAF50; }
.battery-medium { background-color: #ffc107; }
.battery-low { background-color: #f44336; }
.upload-grid { display: grid; grid-template-columns: 1fr; gap: 5px; }
@media (min-width: 600px) { .upload-grid { grid-template-columns: repeat(2, 1fr); } }
.upload-item { background: #f9f9f9; padding: 2px; border-radius: 5px; border: 1px solid #ddd; display: flex; flex-direction: column; }
.upload-item label { font-weight: bold; display: block; margin-bottom: 5px; }
.upload-item .button-group { display: flex; gap: 5px; margin-top: auto; }
.upload-item .button-group button { padding: 4px 8px; font-size: 0.8em; }
input[type="file"] { display: block; margin-bottom: 10px; width: 100%; }
button, .stop-btn { background-color: #007bff; color: white; border: none; padding: 5px 5px; border-radius: 4px; cursor: pointer; font-size: 1em; }
button:hover, .stop-btn:hover { background-color: #0056b3; }
.stop-btn { background-color: #dc3545; }
.stop-btn:hover { background-color: #c82333; }
.file-item { display: flex; justify-content: space-between; align-items: center; padding: 8px; border-bottom: 1px solid #eee; }
.file-status-grid { display: grid; grid-template-columns: repeat(2, 1fr); gap: 8px; }
.file-status-item { background: #f9f9f9; border: 1px solid #eee; padding: 8px; border-radius: 4px; font-size: 0.9em; display: flex; justify-content: space-between; align-items: center; }
.file-status-item > div { display: flex; flex-direction: column; overflow: hidden; }
.file-status-item .filename { font-weight: bold; color: #333; display: block; white-space: nowrap; overflow: hidden; text-overflow: ellipsis; }
.file-status-item .no-file { color: #888; }
.file-status-item button { padding: 4px 8px; font-size: 0.8em; background-color: #dc3545; margin-left: 5px; }
.volume-control { display: flex; align-items: center; gap: 10px; margin-top: 10px; }
.volume-slider { width: 200px; }
.volume-value { min-width: 40px; font-weight: bold; }
.status { margin-top: 15px; padding: 10px; background-color: #e9ecef; border-radius: 4px; }
)=====";

// HTML template for the main page
const char* WEB_HTML = R"=====( 
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

        <div class="top-row">
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
                <div class="volume-control">
                    <label>Volume:</label>
                    <input type="range" id="volume-slider" class="volume-slider" min="0" max="100" value="50" onchange="setVolume(this.value)">
                    <span id="volume-value" class="volume-value">50%</span>
                </div>
            </div>
        </div>
        
        <div class="main-content">
            <div class="section">
                <h2>Audio Files</h2>
                <p class="info">Maximum file size: 500KB per file. Supported format: <b>MP3</b>. Recommended: 64kbps mono or 32kbps stereo for longer audio</p>
                <div id="file-list"></div>
            </div>
            
            <div class="section">
                <h2>Upload Audio Files</h2>
                <form id="upload-form" enctype="multipart/form-data">
                    <div class="upload-grid">
                        <div class="upload-item">
                            <label>Button 1:</label>
                            <input type="file" name="file" data-button="1" accept=".mp3">
                            <div class="button-group">
                                <button type="button" onclick="uploadFile(1)">Upload</button>
                                <button type="button" onclick="testButton(1)">Test</button>
                            </div>
                        </div>
                        <div class="upload-item">
                            <label>Button 2:</label>
                            <input type="file" name="file" data-button="2" accept=".mp3">
                            <div class="button-group">
                                <button type="button" onclick="uploadFile(2)">Upload</button>
                                <button type="button" onclick="testButton(2)">Test</button>
                            </div>
                        </div>
                        <div class="upload-item">
                            <label>Button 3:</label>
                            <input type="file" name="file" data-button="3" accept=".mp3">
                            <div class="button-group">
                                <button type="button" onclick="uploadFile(3)">Upload</button>
                                <button type="button" onclick="testButton(3)">Test</button>
                            </div>
                        </div>
                        <div class="upload-item">
                            <label>Button 4:</label>
                            <input type="file" name="file" data-button="4" accept=".mp3">
                            <div class="button-group">
                                <button type="button" onclick="uploadFile(4)">Upload</button>
                                <button type="button" onclick="testButton(4)">Test</button>
                            </div>
                        </div>
                        <div class="upload-item">
                            <label>Button 5:</label>
                            <input type="file" name="file" data-button="5" accept=".mp3">
                            <div class="button-group">
                                <button type="button" onclick="uploadFile(5)">Upload</button>
                                <button type="button" onclick="testButton(5)">Test</button>
                            </div>
                        </div>
                        <div class="upload-item">
                            <label>Button 6:</label>
                            <input type="file" name="file" data-button="6" accept=".mp3">
                            <div class="button-group">
                                <button type="button" onclick="uploadFile(6)">Upload</button>
                                <button type="button" onclick="testButton(6)">Test</button>
                            </div>
                        </div>
                    </div>
                </form>
            </div>
        </div>
        
        <div id="status">Status messages will appear here.</div>
    </div>

    <script>
        function updateBattery() {
            fetch('/battery')
                .then(response => response.ok ? response.json() : Promise.reject('Network response was not ok.'))
                .then(data => {
                    document.getElementById('battery-voltage').textContent = data.voltage.toFixed(2);
                    const percentage = Math.min(100, Math.max(0, (data.voltage - 3.0) / (4.2 - 3.0) * 100));
                    document.getElementById('battery-level').style.width = percentage + '%';
                    
                    // Color coding
                    const batteryLevel = document.getElementById('battery-level');
                    batteryLevel.classList.remove('battery-good', 'battery-medium', 'battery-low');
                    if (percentage > 50) {
                        batteryLevel.classList.add('battery-good');
                    } else if (percentage > 20) {
                        batteryLevel.classList.add('battery-medium');
                    } else {
                        batteryLevel.classList.add('battery-low');
                    }
                })
                .catch(error => {
                    console.error('Error fetching battery status:', error);
                    document.getElementById('status').textContent = 'Error fetching battery status.';
                });
        }
        
        function updateFileList() {
            fetch('/files')
                .then(response => response.ok ? response.json() : Promise.reject('Network response was not ok.'))
                .then(data => {
                    const fileList = document.getElementById('file-list');
                    fileList.innerHTML = '<div class="file-status-grid"></div>';
                    const grid = fileList.querySelector('.file-status-grid');
                    
                    for (let i = 1; i <= 6; i++) {
                        const filename = 'button' + i + '.mp3';
                        const exists = data.files && data.files.includes(filename);
                        const div = document.createElement('div');
                        div.className = 'file-status-item';
                        let content = `<div><span>Button ${i}</span>`;
                        if (exists) {
                            content += `<span class="filename" title="${filename}">${filename}</span></div><button onclick="deleteFile('${filename}')">Dlt</button>`;
                        } else {
                            content += `<span class="no-file">[No File]</span></div>`;
                        }
                        div.innerHTML = content;
                        grid.appendChild(div);
                    }
                })
                .catch(error => {
                    console.error('Error updating file list:', error);
                    document.getElementById('status').textContent = 'Error updating file list.';
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
            })
            .then(response => response.ok ? response.json() : Promise.reject('Network response was not ok.'))
            .then(data => {
                document.getElementById('status').innerHTML = data.message;
                input.value = '';
                // Update file list after successful upload
                updateFileList();
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
            });
        }
        
        function stopAudio() {
            fetch('/stop', {
                method: 'POST'
            })
            .catch(error => console.error('Error stopping audio:', error));
        }
        
        function deleteFile(filename) {
            if (confirm('Are you sure you want to delete ' + filename + '?')) {
                fetch('/delete', {
                    method: 'POST', 
                    headers: {'Content-Type': 'application/x-www-form-urlencoded'},
                    body: 'filename=' + filename
                })
                .then(response => {
                    if (!response.ok) return Promise.reject('Deletion failed.');
                    document.getElementById('status').innerHTML = `File ${filename} deleted.`;
                    updateFileList();
                })
                .catch(error => document.getElementById('status').innerHTML = error);
            }
        }
        
        function setVolume(value) {
            const volumePercent = parseInt(value);
            const volumeFloat = volumePercent / 100.0;
            
            document.getElementById('volume-value').textContent = volumePercent + '%';
            
            fetch('/volume', {
                method: 'POST',
                headers: {'Content-Type': 'application/x-www-form-urlencoded'},
                body: 'volume=' + volumeFloat
            })
            .catch(error => console.error('Error setting volume:', error));
        }
        
        function getVolume() {
            fetch('/volume')
                .then(response => response.ok ? response.json() : Promise.reject('Network response was not ok.'))
                .then(data => {
                    const volumePercent = Math.round(data.volume * 100);
                    document.getElementById('volume-slider').value = volumePercent;
                    document.getElementById('volume-value').textContent = volumePercent + '%';
                })
                .catch(error => {
                    console.error('Error getting volume:', error);
                    document.getElementById('status').textContent = 'Error getting volume.';
                });
        }
        
        // Initial load and periodic updates
        document.addEventListener('DOMContentLoaded', () => {
            updateBattery();
            updateFileList();
            getVolume();
            setInterval(updateBattery, 10000);
        });
    </script>
</body>
</html>
)=====";

#endif