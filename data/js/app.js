// App JavaScript for Pixel Matrix FX Control

class PixelMatrixApp {
    constructor() {
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
            
            const response = await fetch(`/api/brightness/set?value=${value}`, {
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
            
            const response = await fetch('/api/wifi/reset', {method: 'GET'});
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
            
            const response = await fetch('/api/restart', {method: 'GET'});
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
            const response = await fetch('/api/status', {method: 'GET'});
            const data = await response.json();
            
            if (data.status === 'connected') {
                // Update dynamic content
                const currentGifElement = document.getElementById('current-gif');
                if (currentGifElement && data.current_gif) {
                    currentGifElement.textContent = data.current_gif;
                }
                
                const brightnessElement = document.getElementById('brightness');
                if (brightnessElement && data.brightness) {
                    brightnessElement.textContent = data.brightness;
                }
                
                console.log('Status updated:', data);
            }
        } catch (error) {
            console.error('Status update failed:', error);
            // Don't show error messages for status updates to avoid spam
        }
    }

    // Start periodic status updates
    startStatusUpdates() {
        // Update status every 30 seconds
        setInterval(() => {
            this.updateStatus();
        }, 30000);
        
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

    // Show message to user
    showMessage(message, type = 'info') {
        // Remove existing messages
        const existingMessages = document.querySelectorAll('.message');
        existingMessages.forEach(msg => msg.remove());
        
        // Create new message
        const messageDiv = document.createElement('div');
        messageDiv.className = `message ${type}`;
        messageDiv.textContent = message;
        
        // Insert at the top of main content
        const mainContent = document.querySelector('.main-content');
        if (mainContent) {
            mainContent.insertBefore(messageDiv, mainContent.firstChild);
            
            // Auto-remove after 5 seconds
            setTimeout(() => {
                messageDiv.remove();
            }, 5000);
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
    window.app = new PixelMatrixApp();
});