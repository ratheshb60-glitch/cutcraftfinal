import express from 'express';
import http from 'node:http';
import fs from 'node:fs';
import path from 'node:path';
import { spawn, execSync, type ChildProcess } from 'node:child_process';
import { createServer as createViteServer } from 'vite';
import 'dotenv/config';

const portArg = process.argv.find((a) => a.startsWith('--port='))?.split('=')[1];
const PORT = Number(portArg || process.env.APP_PORT || 3000);
const C_BACKEND_PORT = Number(process.env.CUTCRAFT_C_PORT || 18088);
const C_BINARY_PATH = path.resolve(process.cwd(), 'backend', 'cutcraft_server');

let cProcess: ChildProcess | null = null;

function ensureCBackendBuilt() {
  if (!fs.existsSync(C_BINARY_PATH)) {
    console.log('Compiling CUTCRAFT C backend...');
    execSync('make -C backend', { stdio: 'inherit' });
  }
}

function startCBackend() {
  ensureCBackendBuilt();
  if (cProcess && !cProcess.killed) return;

  cProcess = spawn(C_BINARY_PATH, [String(C_BACKEND_PORT)], {
    cwd: process.cwd(),
    env: {
      ...process.env,
      CUTCRAFT_C_PORT: String(C_BACKEND_PORT),
      CUTCRAFT_DB_PATH: path.resolve(process.cwd(), 'cutcraft.db'),
    },
    stdio: 'inherit',
  });

  cProcess.on('exit', (code) => {
    console.error(`CUTCRAFT C backend exited with code ${code}`);
    cProcess = null;
  });
}

async function waitForCBackend(retries = 30): Promise<void> {
  for (let i = 0; i < retries; i++) {
    const ok = await new Promise<boolean>((resolve) => {
      const req = http.get(`http://127.0.0.1:${C_BACKEND_PORT}/api/health`, (res) => {
        let raw = '';
        res.on('data', (chunk) => { raw += chunk; });
        res.on('end', () => {
          resolve(res.statusCode === 200 && raw.includes('CUTCRAFT C11'));
        });
      });
      req.on('error', () => resolve(false));
      req.setTimeout(500, () => {
        req.destroy();
        resolve(false);
      });
    });
    if (ok) return;
    await new Promise((r) => setTimeout(r, 150));
  }
  throw new Error('CUTCRAFT C backend did not become ready in time.');
}

function proxyToCBackend(req: express.Request, res: express.Response) {
  const options: http.RequestOptions = {
    hostname: '127.0.0.1',
    port: C_BACKEND_PORT,
    path: req.originalUrl,
    method: req.method,
    headers: {
      ...req.headers,
      host: `127.0.0.1:${C_BACKEND_PORT}`,
    },
  };

  const proxyReq = http.request(options, (proxyRes) => {
    res.writeHead(proxyRes.statusCode || 500, proxyRes.headers);
    proxyRes.pipe(res, { end: true });
  });

  proxyReq.on('error', (err) => {
    console.error('C backend proxy error:', err.message);
    if (!res.headersSent) {
      res.status(502).json({ error: 'C backend unavailable. Please retry.' });
    }
  });

  req.pipe(proxyReq, { end: true });
}

async function bootstrap() {
  startCBackend();
  await waitForCBackend();

  const app = express();

  /* Proxy all business-logic API calls and uploaded media directly to the compiled C HTTP + SQLite server */
  app.use('/api', proxyToCBackend);
  app.use('/uploads', proxyToCBackend);

  if (process.env.NODE_ENV !== 'production') {
    const vite = await createViteServer({
      server: { middlewareMode: true },
      appType: 'spa',
    });
    app.use(vite.middlewares);
  } else {
    const distDir = path.resolve(process.cwd(), 'dist');
    app.use(express.static(distDir));
    app.get('*', (_req, res) => {
      res.sendFile(path.join(distDir, 'index.html'));
    });
  }

  app.listen(PORT, '0.0.0.0', () => {
    console.log(`CUTCRAFT Gateway running on http://0.0.0.0:${PORT} -> C backend :${C_BACKEND_PORT}`);
  });
}

process.on('SIGTERM', () => {
  if (cProcess) cProcess.kill('SIGTERM');
  process.exit(0);
});
process.on('SIGINT', () => {
  if (cProcess) cProcess.kill('SIGINT');
  process.exit(0);
});

bootstrap().catch((err) => {
  console.error(err);
  process.exit(1);
});
