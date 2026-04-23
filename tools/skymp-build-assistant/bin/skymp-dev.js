#!/usr/bin/env node
import { main } from '../lib/cli.js';

main().catch((err) => {
  console.error(`[skymp-dev] ${err?.message || String(err)}`);
  process.exitCode = 1;
});
