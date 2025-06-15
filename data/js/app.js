// App JavaScript for Pixel Matrix FX Control

class PixelMatrixApp {

    baseUrl = '';

    constructor(isRunningOnESP) {
        // Determine the base URL based on whether it's running on ESP or locally
        if (!isRunningOnESP) {
            this.baseUrl = 'http://192.168.10.22';
        } else {
            // When running on ESP, paths are relative, so baseUrl can be empty
            this.baseUrl = '';
        }

        console.log('Base URL set to:', this.baseUrl);

        // Call init AFTER baseUrl is set
        this.init();
    }

    init() {
        console.log('Pixel Matrix FX Control App Initialized');
        this.bindEvents();
        this.startStatusUpdates();
    }

    bindEvents() {
        // Any additional event bindings can go here
        // The HTML already has onclick handlers, but we could move them here
    }
   
    // Update brightness display while sliding
    updateBrightnessDisplay(value) {
        document.getElementById('sliderValue').textContent = value;
    }

    // Set brightness via API
    async setBrightness(value) {
        try {
            this.showLoading('brightnessSlider');
            
            const response = await fetch(this.baseUrl + '/api/brightness/set?value='+ value, {
                method: 'GET'
            });
            
            const data = await response.json();
            
            if (data.status === 'success') {
                // Update displays
                document.getElementById('brightness').textContent = data.new_brightness;
                document.getElementById('sliderValue').textContent = data.new_brightness;
                document.getElementById('brightnessSlider').value = data.new_brightness;
                
                this.showMessage('Brightness updated successfully!', 'success');
                console.log('Brightness set to:', data.new_brightness);
            } else {
                this.showMessage('Error: ' + data.message, 'error');
            }
        } catch (error) {
            this.showMessage('Request failed: ' + error.message, 'error');
            console.error('Brightness request failed:', error);
        } finally {
            this.hideLoading('brightnessSlider');
        }
    }

    // Reset WiFi credentials
    async resetWiFi() {
        if (!confirm('Are you sure you want to reset WiFi credentials? Device will restart in config mode.')) {
            return;
        }

        try {
            this.showLoading('resetWiFiBtn');
            
            const response = await fetch(this.baseUrl + '/api/wifi/reset', {method: 'GET'});
            const data = await response.json();
            
            this.showMessage(data.message, data.status === 'success' ? 'success' : 'error');
            
            if (data.status === 'success') {
                setTimeout(() => {
                    this.showMessage('Device is restarting... You will need to reconnect.', 'info');
                }, 1000);
            }
        } catch (error) {
            this.showMessage('Request failed: ' + error.message, 'error');
            console.error('WiFi reset request failed:', error);
        } finally {
            this.hideLoading('resetWiFiBtn');
        }
    }

    // Restart device
    async restartDevice() {
        if (!confirm('Are you sure you want to restart the device?')) {
            return;
        }

        try {
            this.showLoading('restartBtn');
            
            const response = await fetch(this.baseUrl + '/api/restart', {method: 'GET'});
            const data = await response.json();
            
            this.showMessage(data.message, data.status === 'success' ? 'success' : 'error');
            
            if (data.status === 'success') {
                setTimeout(() => {
                    this.showMessage('Device is restarting... Page will reload automatically.', 'info');
                    // Attempt to reload page after restart
                    setTimeout(() => {
                        window.location.reload();
                    }, 10000);
                }, 1000);
            }
        } catch (error) {
            this.showMessage('Request failed: ' + error.message, 'error');
            console.error('Restart request failed:', error);
        } finally {
            this.hideLoading('restartBtn');
        }
    }

    // Get device status and update UI
    async updateStatus() {
        try {
            const response = await fetch(this.baseUrl + '/api/status', {method: 'GET'});
            const data = await response.json();
            
            if (data.status === 'connected') {
                // Update dynamic content
                const ssidElement = document.getElementById('ssid');
                if (ssidElement && data.ssid) {
                    ssidElement.textContent = data.ssid;
                }

                const ipElement = document.getElementById('ip');
                if (ipElement && data.ip) {
                    ipElement.textContent = data.ip;
                }

                const currentGifElement = document.getElementById('current-gif');
                if (currentGifElement && data.current_gif) {
                    currentGifElement.textContent = data.current_gif;
                }
                
                const brightnessElement = document.getElementById('brightness');
                if (brightnessElement && data.brightness) {
                    brightnessElement.textContent = data.brightness;
                }
                
                // Update the status bar as well
                this.showMessage('Connected', 'connected', true); // Use 'connected' type for green
                console.log('Status updated:', data);
            } else {
                this.showMessage('Disconnected', 'error', true); // Use 'error' type for yellow
            }
        } catch (error) {
            console.error('Status update failed:', error);
            this.showMessage('Disconnected', 'error', true); // Show disconnected if status update fails
        }
    }

    // Start periodic status updates
    startStatusUpdates() {
        // Update status every 10 seconds
        setInterval(() => {
            this.updateStatus();
        }, 10000);
        
        // Initial status update
        this.updateStatus();
    }

    // Show loading state
    showLoading(elementId) {
        const element = document.getElementById(elementId);
        if (element) {
            element.classList.add('loading');
            element.disabled = true;
        }
    }

    // Hide loading state
    hideLoading(elementId) {
        const element = document.getElementById(elementId);
        if (element) {
            element.classList.remove('loading');
            element.disabled = false;
        }
    }

    // Show message to user using the status bar
    showMessage(message, type = 'info', isStatusBarUpdate = false) {
        const statusValueElement = document.querySelector('.status-bar .status-value');
        if (statusValueElement) {
            statusValueElement.textContent = message;
            // Remove previous type classes
            statusValueElement.classList.remove('connected', 'error', 'success', 'info');
            // Add the new type class
            statusValueElement.classList.add(type);

            // If it's a transient message (not a continuous status update), remove it after a delay
            if (!isStatusBarUpdate) {
                setTimeout(() => {
                    // Reset to "Connected" or last known good status after transient message
                    this.updateStatus(); // Re-fetch current status to display
                }, 5000); // Message disappears after 5 seconds
            }
        }
    }
}

// Global functions for HTML onclick handlers
function updateBrightnessDisplay(value) {
    window.app.updateBrightnessDisplay(value);
}

function setBrightness(value) {
    window.app.setBrightness(value);
}

function resetWiFi() {
    window.app.resetWiFi();
}

function restartDevice() {
    window.app.restartDevice();
}

// Initialize app when DOM is loaded
document.addEventListener('DOMContentLoaded', function() {
    window.app = new PixelMatrixApp(false);
});