# Environment Detection for Pixel Matrix FX Control

## Overview

This system uses separate environment files to automatically detect whether the web interface is running on the ESP32 or locally for development.

## Files

- **`environment.js`** - Used for local development (`isRunningOnESP = false`)
- **`environment-esp32.js`** - Used on ESP32 (`isRunningOnESP = true`)

## How It Works

1. **Local Development**: 
   - Uses `environment.js` with `isRunningOnESP = false`
   - API calls go to `http://localhost:8080`

2. **ESP32 Deployment**:
   - Uses `environment-esp32.js` with `isRunningOnESP = true`
   - API calls use relative URLs (same origin)

## Scripts

### Copy to SD Card (`_copy_web_files.ps1`)
- Excludes `environment.js` from copying
- Copies `environment-esp32.js` as `environment.js` to the SD card
- This ensures the ESP32 gets the correct environment setting

### Upload to ESP32 (`_upload_web_files.ps1`)
- Excludes `environment.js` from uploading
- Uploads `environment-esp32.js` as `environment.js` to the ESP32
- This ensures the ESP32 gets the correct environment setting

## Usage

### For Local Development
1. Keep `environment.js` with `isRunningOnESP = false`
2. Run your local development server on port 8080
3. Access the web interface locally

### For ESP32 Deployment
1. Run `_copy_web_files.ps1` to copy files to SD card
2. OR run `_upload_web_files.ps1` to upload files to ESP32
3. The scripts automatically handle the environment file switching

## Manual Override

If you need to manually change the environment:

**For Local Development:**
```javascript
// In data/js/environment.js
const isRunningOnESP = false;
```

**For ESP32:**
```javascript
// In data/js/environment-esp32.js
const isRunningOnESP = true;
```

The scripts will automatically use the correct file based on the deployment method. 