const express = require('express');
const fs = require('fs');
const path = require('path');
const multer = require('multer');
const cors = require('cors');

const app = express();
const PORT = 8080;
const BASE_DIR = 'c:/dev';

app.use(cors());
app.use(express.json());

// File upload setup
const upload = multer({ dest: path.join(BASE_DIR, 'uploads/') });

// Device Management Endpoints (mocked)
app.get('/api/status', (req, res) => {
  res.json({ status: 'connected', ssid: 'MockSSID', ip: '127.0.0.1', current_gif: '', free_heap: 123456, uptime: 123456, brightness: 128 });
});
app.get('/api/brightness/increase', (req, res) => res.json({ status: 'success', message: 'Brightness increased', old_brightness: 100, new_brightness: 125 }));
app.get('/api/brightness/decrease', (req, res) => res.json({ status: 'success', message: 'Brightness decreased', old_brightness: 125, new_brightness: 100 }));
app.get('/api/brightness/set', (req, res) => res.json({ status: 'success', message: 'Brightness set', old_brightness: 100, new_brightness: req.query.value }));
app.get('/api/wifi/reset', (req, res) => res.json({ status: 'success', message: 'WiFi reset' }));
app.get('/api/restart', (req, res) => res.json({ status: 'success', message: 'Device restarting' }));

// GIF upload (accepts .gif only)
app.post('/api/gif/upload', upload.single('file'), (req, res) => {
  const file = req.file;
  const filename = req.body.filename || (file ? file.originalname : '');
  if (!filename.endsWith('.gif')) {
    if (file) fs.unlinkSync(file.path);
    return res.status(400).json({ status: 'error', message: 'Only .gif files are allowed' });
  }
  // Move file to gifs folder
  const gifsDir = path.join(BASE_DIR, 'gifs');
  if (!fs.existsSync(gifsDir)) fs.mkdirSync(gifsDir);
  const dest = path.join(gifsDir, filename);
  fs.renameSync(file.path, dest);
  res.json({ status: 'success', message: 'Upload complete' });
});

// File Management Endpoints
app.get('/api/files', (req, res) => {
  const dirPath = path.join(BASE_DIR, req.query.path || '/');
  if (!fs.existsSync(dirPath) || !fs.lstatSync(dirPath).isDirectory()) {
    return res.status(400).json({ status: 'error', message: 'Directory not found' });
  }
  const files = fs.readdirSync(dirPath).map(name => {
    const stat = fs.lstatSync(path.join(dirPath, name));
    return {
      name,
      type: stat.isDirectory() ? 'folder' : 'file',
      size: stat.isDirectory() ? undefined : stat.size
    };
  });
  res.json(files);
});

app.post('/api/files/delete', (req, res) => {
  const { path: filePath } = req.body;
  const absPath = path.join(BASE_DIR, filePath);
  if (!fs.existsSync(absPath)) return res.status(404).json({ status: 'error', message: 'File/folder not found' });
  try {
    if (fs.lstatSync(absPath).isDirectory()) fs.rmdirSync(absPath, { recursive: true });
    else fs.unlinkSync(absPath);
    res.json({ status: 'success', message: 'Deleted' });
  } catch {
    res.status(500).json({ status: 'error', message: 'Delete failed' });
  }
});

app.post('/api/files/rename', (req, res) => {
  const { path: oldPath, newName } = req.body;
  const absOld = path.join(BASE_DIR, oldPath);
  const absNew = path.join(path.dirname(absOld), newName);
  if (!fs.existsSync(absOld)) return res.status(404).json({ status: 'error', message: 'File/folder not found' });
  try {
    fs.renameSync(absOld, absNew);
    res.json({ status: 'success', message: 'Renamed' });
  } catch {
    res.status(500).json({ status: 'error', message: 'Rename failed' });
  }
});

app.post('/api/files/move', (req, res) => {
  const { path: oldPath, newPath } = req.body;
  const absOld = path.join(BASE_DIR, oldPath);
  const absNew = path.join(BASE_DIR, newPath);
  if (!fs.existsSync(absOld)) return res.status(404).json({ status: 'error', message: 'File/folder not found' });
  try {
    fs.renameSync(absOld, absNew);
    res.json({ status: 'success', message: 'Moved' });
  } catch {
    res.status(500).json({ status: 'error', message: 'Move failed' });
  }
});

app.post('/api/files/create-folder', (req, res) => {
  const { name } = req.body;
  const absPath = path.join(BASE_DIR, name);
  if (fs.existsSync(absPath)) return res.status(409).json({ status: 'error', message: 'Folder already exists' });
  try {
    fs.mkdirSync(absPath);
    res.json({ status: 'success', message: 'Folder created' });
  } catch {
    res.status(500).json({ status: 'error', message: 'Create folder failed' });
  }
});

app.post('/api/dirs/rename', (req, res) => {
  const { path: oldPath, newName } = req.body;
  const absOld = path.join(BASE_DIR, oldPath);
  const absNew = path.join(path.dirname(absOld), newName);
  if (!fs.existsSync(absOld)) return res.status(404).json({ status: 'error', message: 'Directory not found' });
  try {
    fs.renameSync(absOld, absNew);
    res.json({ status: 'success', message: 'Directory renamed' });
  } catch {
    res.status(500).json({ status: 'error', message: 'Rename failed' });
  }
});

app.post('/api/dirs/delete', (req, res) => {
  const { path: dirPath } = req.body;
  const absPath = path.join(BASE_DIR, dirPath);
  if (!fs.existsSync(absPath)) return res.status(404).json({ status: 'error', message: 'Directory not found' });
  try {
    fs.rmdirSync(absPath, { recursive: true });
    res.json({ status: 'success', message: 'Directory deleted' });
  } catch {
    res.status(500).json({ status: 'error', message: 'Delete failed' });
  }
});

app.listen(PORT, () => {
  console.log(`Node webserver running at http://localhost:${PORT}`);
});
