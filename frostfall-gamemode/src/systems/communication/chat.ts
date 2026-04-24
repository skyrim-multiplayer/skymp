// ── Chat System ───────────────────────────────────────────────────────────────
//
// Channels
//   IC (default)   proximity speech within SAY_RANGE
//   /me            roleplay action, proximity
//   /ooc           global out-of-character
//   /w <name>      private whisper (must be within WHISPER_RANGE)
//   /f             faction members only
//
// Server → Client flow
//   deliver() → mp.set(actorId, 'chatMsg', '#{rrggbb}text…') → updateOwner
//   → executeJavaScript → parses #{color} codes → widgets.set → React re-render
//
// Client → Server flow
//   Chat input → window.mp.send('cef::chat:send', text) → customPacket event
//   → index.ts mp.on('customPacket', …) → handleChatInput()
//
// Reload resilience
//   History is maintained per-player.  If a reload trigger is wired (e.g. via
//   makeEventSource or another customPacket handler), callers can invoke
//   handleChatInput(mp, store, userId, '__reload__') to replay recent messages.
//
// Public API
//   init(mp)
//   handleChatInput(mp, store, userId, text): boolean  — true = consumed
//   sendSystem(mp, store, userId, text)
//   broadcastSystem(mp, store, text)
//   sendToPlayer(mp, store, userId, text, color?)      — legacy plain-text
//   broadcast(mp, store, text, color?)                 — legacy plain-text broadcast

import { signScript } from '../../core/signHelper'
import type { Mp, Store } from '../../types'

// ── Config ────────────────────────────────────────────────────────────────────

const CHAT_MSG_PROP  = 'chatMsg'
const SAY_RANGE      = 3500    // Skyrim units ≈ 50 m
const WHISPER_RANGE  = 400     // units ≈ 6 m
export const MAX_MSG_LEN = 300
const MAX_HISTORY    = 30      // msgs kept per player for reload replay
const RATE_LIMIT_MS  = 1000   // minimum ms between messages per player

// ── Client-side bridge ────────────────────────────────────────────────────────
//
// updateOwner runs in the Skyrim Platform Chakra context (ES5-safe) whenever
// the server sets a new value on 'chatMsg'.
//
// Message format: "#{rrggbb}segment1#{rrggbb}segment2…"
// The Chakra wrapper reads ctx.value, JSON-encodes it for safe embedding, then
// passes it to executeJavaScript.  The browser-side (CEF/Chromium) code parses
// the #{color} codes into Span segments and pushes a new ChatMsg into
// window.chatMessages, then refreshes the widget.
//
// The chat widget's send() calls window.mp.send('cef::chat:send', text) which
// the server receives as a customPacket event.

const UPDATE_OWNER_JS = `
(function(){
  var rawMsg=String(ctx.value||"");
  if(!rawMsg)return;
  var safeMsg=JSON.stringify(rawMsg);
  ctx.sp.browser.executeJavaScript(
    "(function(){"+
    "  try{"+
    "    var raw="+safeMsg+";"+
    "    if(!window.chatMessages)window.chatMessages=[];"+
    "    var segs=[];"+
    "    var rem=raw;"+
    "    var col='#fafafa';"+
    "    var reColor=/^#\\\\{([0-9a-fA-F]{6})\\\\}/;"+
    "    while(rem.length>0){"+
    "      var m=rem.match(reColor);"+
    "      if(m){col='#'+m[1];rem=rem.slice(m[0].length);continue;}"+
    "      var n=rem.indexOf('#{');"+
    "      if(n<0){segs.push({text:rem,color:col,opacity:1,type:['default']});rem='';break;}"+
    "      if(n>0)segs.push({text:rem.slice(0,n),color:col,opacity:1,type:['default']});"+
    "      rem=rem.slice(n);"+
    "    }"+
    "    if(segs.length===0)return;"+
    "    window.chatMessages.push({text:segs,category:'default',opacity:1});"+
    "    while(window.chatMessages.length>50)window.chatMessages.shift();"+
    "    if(window.skyrimPlatform&&window.skyrimPlatform.widgets){"+
    "      var ws=window.skyrimPlatform.widgets.get();"+
    "      var found=false;"+
    "      var newWs=ws.map(function(w){"+
    "        if(w.type==='chat'){found=true;return Object.assign({},w,{messages:window.chatMessages.slice()});}"+
    "        return w;"+
    "      });"+
    "      if(!found){"+
    "        newWs.push({"+
    "          type:'chat',"+
    "          messages:window.chatMessages.slice(),"+
    "          send:function(m){window.mp&&window.mp.send('cef::chat:send',m);}"+
    "        });"+
    "      }"+
    "      window.skyrimPlatform.widgets.set(newWs);"+
    "    }"+
    "    window.needToScroll=true;"+
    "    if(typeof window.scrollToLastMessage==='function')window.scrollToLastMessage();"+
    "  }catch(e){}"+
    "})();"
  );
})();
`.trim()

// ── Color palette ─────────────────────────────────────────────────────────────

const C = {
  nameIc:      '#e8c87a',  // golden  — IC speaker
  nameOoc:     '#8888bb',  // slate   — OOC speaker
  nameFaction: '#66bb66',  // green   — faction chat
  nameWhisper: '#bb88cc',  // purple  — whisper
  nameSystem:  '#ff9933',  // orange  — [System] prefix
  tagIc:       '#666666',  // dim     — [Say] (unused, kept for future)
  tagOoc:      '#444466',  // dim     — [OOC] tag
  tagFaction:  '#335533',  // dim grn — [Faction] tag
  tagWhisper:  '#553366',  // dim pur — [Whisper] tag
  msgIc:       '#ffffff',  // white   — IC speech
  msgOoc:      '#ccccdd',  // lavender— OOC text
  msgMe:       '#ccccbb',  // pale    — /me action text
  msgWhisper:  '#cc99ff',  // light pur
  msgFaction:  '#aaddaa',  // light grn
  system:      '#ffcc44',  // gold    — system body
}

// ── Types ─────────────────────────────────────────────────────────────────────

interface Span    { text: string; color: string; opacity: number; type: string[] }
interface ChatMsg { category: string; text: Span[]; opacity: number }

function sp(text: string, color: string, types: string[] = ['text']): Span {
  return { text, color, opacity: 1, type: types }
}

function mkMsg(category: 'plain' | 'rp', ...spans: Span[]): ChatMsg {
  return { category, text: spans, opacity: 1 }
}

// Converts the server-side Span array to the "#{rrggbb}text" wire format.
// Colors in the palette are "#rrggbb"; we strip the leading "#" for the tag.
function spansToColorString(spans: Span[]): string {
  return spans.map(s => `#{${s.color.slice(1)}}${s.text}`).join('')
}

// ── Per-player recent-message history (for reload replay) ─────────────────────

const playerHistory = new Map<number, ChatMsg[]>()
const lastMsgTime   = new Map<number, number>()

function pushHistory(userId: number, m: ChatMsg): void {
  const h = playerHistory.get(userId) ?? []
  h.push(m)
  if (h.length > MAX_HISTORY) h.shift()
  playerHistory.set(userId, h)
}

function replayHistory(mp: Mp, store: Store, userId: number): void {
  const player = store.get(userId)
  if (!player) return
  const history = playerHistory.get(userId) ?? []
  for (const m of history) {
    deliver(mp, player.actorId, m)
  }
}

// ── Delivery ──────────────────────────────────────────────────────────────────

function deliver(mp: Mp, actorId: number, m: ChatMsg): void {
  if (!actorId) return
  try {
    mp.set(actorId, CHAT_MSG_PROP, spansToColorString(m.text))
  } catch {
    // actor not ready yet — silently skip
  }
}

// ── Proximity helper ──────────────────────────────────────────────────────────

function dist3(
  a: [number, number, number] | null,
  b: [number, number, number] | null,
): number {
  if (!a || !b) return Infinity
  return Math.sqrt((a[0] - b[0]) ** 2 + (a[1] - b[1]) ** 2 + (a[2] - b[2]) ** 2)
}

function sendProximity(
  mp: Mp,
  store: Store,
  senderActorId: number,
  m: ChatMsg,
  range: number,
): void {
  const origin = mp.getActorPos(senderActorId)
  for (const p of store.getAll()) {
    if (dist3(origin, mp.getActorPos(p.actorId)) <= range) {
      deliver(mp, p.actorId, m)
      pushHistory(p.id, m)
    }
  }
}

// ── init ──────────────────────────────────────────────────────────────────────

export function init(mp: Mp): void {
  mp.makeProperty(CHAT_MSG_PROP, {
    isVisibleByOwner:    true,
    isVisibleByNeighbors: false,
    updateOwner:         signScript(UPDATE_OWNER_JS),
    updateNeighbor:      '',
  })

  console.log('[chat] property registered')
}

// ── handleChatInput ───────────────────────────────────────────────────────────
//
// Returns true  → input was consumed (chat channel or IC speech, or __reload__)
// Returns false → not a chat channel; caller should route to commands

export function handleChatInput(
  mp: Mp,
  store: Store,
  userId: number,
  text: string,
): boolean {
  // ── Special reload sentinel ───────────────────────────────────────────────
  if (text === '__reload__') {
    replayHistory(mp, store, userId)
    return true
  }

  const player = store.get(userId)
  if (!player) return true  // player not registered yet, silently consume

  // ── Server-side rate limiting ─────────────────────────────────────────────
  const now = Date.now()
  const last = lastMsgTime.get(userId) ?? 0
  if (now - last < RATE_LIMIT_MS) {
    const rateMsg = mkMsg('plain',
      sp('[System] ', C.nameSystem, ['nonrp']),
      sp('Please wait before sending another message.', C.system, ['nonrp', 'text']),
    )
    deliver(mp, player.actorId, rateMsg)
    return true
  }
  lastMsgTime.set(userId, now)

  // Strip control characters to prevent rendering artifacts
  const raw = text.trim().replace(/[\x00-\x1F\x7F]/g, '')
  if (!raw || raw.length > MAX_MSG_LEN) return true

  const lower = raw.toLowerCase()

  // ── /me <action> ─────────────────────────────────────────────────────────
  if (lower.startsWith('/me ')) {
    const action = raw.slice(4).trim()
    if (!action) return true
    const m = mkMsg('rp',
      sp('* ', C.tagIc, ['nonrp']),
      sp(player.name, C.nameIc, ['nonrp']),
      sp(' ' + action + ' *', C.msgMe, ['rp']),
    )
    sendProximity(mp, store, player.actorId, m, SAY_RANGE)
    console.log(`[chat:me] ${player.name} ${action}`)
    return true
  }

  // ── /ooc <text> ───────────────────────────────────────────────────────────
  if (lower.startsWith('/ooc ') || lower === '/ooc') {
    const body = raw.slice(5).trim()
    if (!body) return true
    const m = mkMsg('plain',
      sp('[OOC] ', C.tagOoc, ['nonrp']),
      sp(player.name + ': ', C.nameOoc, ['nonrp']),
      sp(body, C.msgOoc, ['nonrp', 'text']),
    )
    for (const p of store.getAll()) {
      deliver(mp, p.actorId, m)
      pushHistory(p.id, m)
    }
    console.log(`[chat:ooc] ${player.name}: ${body}`)
    return true
  }

  // ── /w <name> <text> ──────────────────────────────────────────────────────
  if (lower.startsWith('/w ')) {
    const rest     = raw.slice(3).trim()
    const spaceIdx = rest.indexOf(' ')
    if (spaceIdx === -1) return true
    const targetName = rest.slice(0, spaceIdx).toLowerCase()
    const body       = rest.slice(spaceIdx + 1).trim()
    if (!body) return true

    const target = store.getAll().find(p => p.name.toLowerCase() === targetName)
    if (!target) {
      const notFound = mkMsg('plain',
        sp('[Whisper] ', C.tagWhisper, ['nonrp']),
        sp(`Player "${rest.slice(0, spaceIdx)}" is not online.`, C.system, ['nonrp', 'text']),
      )
      deliver(mp, player.actorId, notFound)
      return true
    }

    const d = dist3(mp.getActorPos(player.actorId), mp.getActorPos(target.actorId))
    if (d > WHISPER_RANGE) {
      const tooFar = mkMsg('plain',
        sp('[Whisper] ', C.tagWhisper, ['nonrp']),
        sp('Too far away to whisper.', C.system, ['nonrp', 'text']),
      )
      deliver(mp, player.actorId, tooFar)
      return true
    }

    const toTarget = mkMsg('plain',
      sp('[Whisper] ', C.tagWhisper, ['nonrp']),
      sp(player.name + ' whispers: ', C.nameWhisper, ['nonrp']),
      sp(body, C.msgWhisper, ['text']),
    )
    const toSelf = mkMsg('plain',
      sp('[→ ' + target.name + '] ', C.tagWhisper, ['nonrp']),
      sp(body, C.msgWhisper, ['text']),
    )
    deliver(mp, target.actorId, toTarget)
    pushHistory(target.id, toTarget)
    deliver(mp, player.actorId, toSelf)
    pushHistory(player.id, toSelf)
    console.log(`[chat:whisper] ${player.name} → ${target.name}: ${body}`)
    return true
  }

  // ── /f <text> (faction chat) ──────────────────────────────────────────────
  if (lower.startsWith('/f ') || lower === '/f') {
    const body = raw.slice(3).trim()
    if (!body) return true

    if (!player.factions.length) {
      const noFaction = mkMsg('plain',
        sp('[Faction] ', C.tagFaction, ['nonrp']),
        sp('You are not in a faction.', C.system, ['nonrp', 'text']),
      )
      deliver(mp, player.actorId, noFaction)
      return true
    }

    const m = mkMsg('plain',
      sp('[Faction] ', C.tagFaction, ['nonrp']),
      sp(player.name + ': ', C.nameFaction, ['nonrp']),
      sp(body, C.msgFaction, ['text']),
    )
    for (const p of store.getAll()) {
      if (p.factions.some(f => player.factions.includes(f))) {
        deliver(mp, p.actorId, m)
        pushHistory(p.id, m)
      }
    }
    console.log(`[chat:faction] ${player.name}: ${body}`)
    return true
  }

  // ── Unknown /command → let caller route to command handler ───────────────
  if (raw.startsWith('/')) return false

  // ── IC (proximity speech, default) ───────────────────────────────────────
  const m = mkMsg('plain',
    sp(player.name + ': ', C.nameIc, ['text']),
    sp(raw, C.msgIc, ['text']),
  )
  sendProximity(mp, store, player.actorId, m, SAY_RANGE)
  console.log(`[chat:ic] ${player.name}: ${raw}`)
  return true
}

// ── Named API ─────────────────────────────────────────────────────────────────

/**
 * Send a styled [System] message to a single player.
 */
export function sendSystem(mp: Mp, store: Store, userId: number, text: string): void {
  const player = store.get(userId)
  if (!player) return
  const m = mkMsg('plain',
    sp('[System] ', C.nameSystem, ['nonrp']),
    sp(text, C.system, ['nonrp', 'text']),
  )
  deliver(mp, player.actorId, m)
  pushHistory(userId, m)
}

/**
 * Broadcast a styled [System] message to all connected players.
 */
export function broadcastSystem(mp: Mp, store: Store, text: string): void {
  const m = mkMsg('plain',
    sp('[System] ', C.nameSystem, ['nonrp']),
    sp(text, C.system, ['nonrp', 'text']),
  )
  for (const p of store.getAll()) {
    deliver(mp, p.actorId, m)
    pushHistory(p.id, m)
  }
  console.log(`[chat:system] ${text}`)
}

/**
 * Send a plain-text message to a single player.
 * Kept for backward compatibility with other systems that call this directly.
 */
export function sendToPlayer(
  mp: Mp,
  store: Store,
  userId: number,
  text: string,
  color = '#ffffff',
): void {
  const player = store.get(userId)
  if (!player) return
  const m = mkMsg('plain', sp(text, color, ['text']))
  deliver(mp, player.actorId, m)
  pushHistory(userId, m)
}

/**
 * Broadcast a plain-text message to all connected players.
 * Kept for backward compatibility.
 */
export function broadcast(mp: Mp, store: Store, text: string, color = '#ffffff'): void {
  const m = mkMsg('plain', sp(text, color, ['text']))
  for (const p of store.getAll()) {
    deliver(mp, p.actorId, m)
    pushHistory(p.id, m)
  }
}
