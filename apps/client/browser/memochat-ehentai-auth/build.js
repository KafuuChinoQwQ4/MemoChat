// Build script to copy files to dist directory
import * as fs from 'fs';
import * as path from 'path';
import { fileURLToPath } from 'url';

const __filename = fileURLToPath(import.meta.url);
const __dirname = path.dirname(__filename);

const distDir = path.join(__dirname, 'dist');
const publicDir = path.join(__dirname, 'public');

// Ensure dist directory exists
if (!fs.existsSync(distDir)) {
  fs.mkdirSync(distDir, { recursive: true });
}

// Copy manifest and HTML files
const filesToCopy = ['manifest.json', 'popup.html'];

for (const file of filesToCopy) {
  const srcPath = path.join(publicDir, file);
  const destPath = path.join(distDir, file);

  if (fs.existsSync(srcPath)) {
    fs.copyFileSync(srcPath, destPath);
    console.log(`Copied ${file}`);
  }
}

// Create placeholder icons (in production, use real icons)
const iconSizes = [16, 48, 128];
for (const size of iconSizes) {
  const iconPath = path.join(distDir, `icon${size}.png`);
  if (!fs.existsSync(iconPath)) {
    // Create a minimal 1x1 transparent PNG as placeholder
    const pngData = Buffer.from(
      'iVBORw0KGgoAAAANSUhEUgAAAAEAAAABCAYAAAAfFcSJAAAADUlEQVR42mNk+M9QDwADhgGAWjR9awAAAABJRU5ErkJggg==',
      'base64'
    );
    fs.writeFileSync(iconPath, pngData);
    console.log(`Created placeholder icon${size}.png`);
  }
}

console.log('Build complete!');
