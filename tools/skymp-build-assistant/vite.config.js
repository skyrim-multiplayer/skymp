import path from 'path';
import { fileURLToPath } from 'url';

import react from '@vitejs/plugin-react';
import { defineConfig } from 'vite';

const __dirname = path.dirname(fileURLToPath(import.meta.url));

export default defineConfig({
  root: path.join(__dirname, 'ui'),
  plugins: [react()],
  build: {
    outDir: path.join(__dirname, 'ui', 'dist'),
    emptyOutDir: true,
  },
});
