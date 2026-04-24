// ── Runtime Global Probe ───────────────────────────────────────────────────────
//
// Checks which networking and I/O globals SkyMP's Chakra sandbox exposes.
// Run once at startup (gated by PROBE_GLOBALS=1 env var or dev mode).
//
// Results appear in the SkyMP server console — grep for [probe].
//
// What we're looking for:
//   fetch          → can make outbound HTTP from gamemode directly
//   WebSocket      → native WS client support
//   XMLHttpRequest → legacy XHR
//   require        → Node.js module system (would mean actual Node, not Chakra)
//   process        → Node.js process object
//   http / https   → Node.js http modules already required
//   setInterval    → timer support (needed for polling fallbacks)

type Availability = 'function' | 'object' | 'missing' | 'other'

function probe(name: string, value: unknown): void {
  let status: Availability
  if (value === undefined || value === null) {
    status = 'missing'
  } else if (typeof value === 'function') {
    status = 'function'
  } else if (typeof value === 'object') {
    status = 'object'
  } else {
    status = 'other'
  }
  console.log(`[probe] ${name.padEnd(20)} → ${status}`)
}

async function attemptFetch(): Promise<void> {
  // Use httpbin as a neutral echo endpoint — safe, no auth, returns JSON.
  const url = 'https://httpbin.org/get'
  try {
    const g = globalThis as any
    if (typeof g.fetch !== 'function') {
      console.log('[probe] fetch live test       → skipped (not a function)')
      return
    }
    console.log('[probe] fetch live test       → attempting GET ' + url)
    const res  = await g.fetch(url)
    const json = await res.json() as Record<string, unknown>
    console.log('[probe] fetch live test       → OK  status=' + res.status)
    console.log('[probe] fetch response origin → ' + String(json['origin'] ?? '(none)'))
  } catch (err: any) {
    console.log('[probe] fetch live test       → FAILED  ' + String(err?.message ?? err))
  }
}

/**
 * Run all runtime global probes and log results to the SkyMP console.
 * Call this from index.ts init() gated by a dev flag.
 */
export async function runGlobalProbes(): Promise<void> {
  const g = globalThis as any

  console.log('[probe] ── SkyMP runtime global probe ──────────────────────────')

  // Networking
  probe('fetch',              g.fetch)
  probe('WebSocket',          g.WebSocket)
  probe('XMLHttpRequest',     g.XMLHttpRequest)

  // Node.js indicators
  probe('require',            g.require)
  probe('process',            g.process)
  probe('Buffer',             g.Buffer)

  // Node built-in modules (only resolvable if actual Node.js)
  try   { probe('require("http")',  g.require?.('http'))  }
  catch { probe('require("http")',  undefined) }
  try   { probe('require("https")', g.require?.('https')) }
  catch { probe('require("https")', undefined) }
  try   { probe('require("net")',   g.require?.('net'))   }
  catch { probe('require("net")',   undefined) }

  // Timer support (important for any polling fallback)
  probe('setInterval',        g.setInterval)
  probe('setTimeout',         g.setTimeout)
  probe('clearInterval',      g.clearInterval)
  probe('Promise',            g.Promise)

  console.log('[probe] ── live fetch attempt ────────────────────────────────────')
  await attemptFetch()

  console.log('[probe] ── WebSocket live test ───────────────────────────────────')
  attemptWebSocket()

  console.log('[probe] ── done (ws result will appear asynchronously) ──────────')
}

// Test whether the WebSocket constructor actually fires events.
// Connects to ws://localhost:7778 — start the backend relay before running.
function attemptWebSocket(): void {
  const g = globalThis as any
  if (typeof g.WebSocket !== 'function') {
    console.log('[probe] WebSocket live test    → skipped (not a function)')
    return
  }
  try {
    const ws = new g.WebSocket('ws://localhost:7778')
    console.log('[probe] WebSocket constructed  → readyState=' + ws.readyState)
    ws.onopen    = () => console.log('[probe] WebSocket onopen       → FIRED (readyState=' + ws.readyState + ')')
    ws.onclose   = (e: any) => console.log('[probe] WebSocket onclose      → FIRED code=' + e?.code)
    ws.onerror   = (e: any) => console.log('[probe] WebSocket onerror      → FIRED ' + String(e?.message ?? e))
    ws.onmessage = (e: any) => console.log('[probe] WebSocket onmessage    → FIRED data=' + String(e?.data).slice(0, 80))
    // If no event fires within 5 s, the event loop likely doesn't tick WS callbacks
    setTimeout(() => {
      console.log('[probe] WebSocket 5s check     → readyState=' + ws.readyState + ' (0=CONNECTING 1=OPEN 2=CLOSING 3=CLOSED)')
    }, 5000)
  } catch (err: any) {
    console.log('[probe] WebSocket live test    → FAILED (constructor threw) ' + String(err?.message ?? err))
  }
}
