// ── Shared types and protocol constants for skymp5-chat ──────────────────────

/** A single colored text segment in a chat message. */
export interface Span {
  text: string
  color: string
  opacity: number
  type: string[]
}

/** A full chat message composed of colored segments. */
export interface ChatMsg {
  category: 'plain' | 'rp'
  text: Span[]
  opacity: number
}

/** Keep the frontend input limit aligned with server validation. */
export const MAX_CHAT_MESSAGE_LENGTH = 300

/** Available chat channels. */
export const enum ChatChannel {
  IC,
  OOC,
  Me,
  Whisper,
  Faction,
  System,
}

/** Server-side color palette. */
export const COLORS = {
  nameIc:      '#e8c87a',  // golden  — IC speaker name
  nameOoc:     '#8888bb',  // slate   — OOC speaker name
  nameFaction: '#66bb66',  // green   — faction chat name
  nameWhisper: '#bb88cc',  // purple  — whisper name
  nameSystem:  '#ff9933',  // orange  — [System] prefix
  tagIc:       '#666666',  // dim     — [Say] tag
  tagOoc:      '#444466',  // dim     — [OOC] tag
  tagFaction:  '#335533',  // dim grn — [Faction] tag
  tagWhisper:  '#553366',  // dim pur — [Whisper] tag
  msgIc:       '#ffffff',  // white   — IC speech
  msgOoc:      '#ccccdd',  // lavender— OOC text
  msgMe:       '#ccccbb',  // pale    — /me action
  msgWhisper:  '#cc99ff',  // light pur
  msgFaction:  '#aaddaa',  // light grn
  system:      '#ffcc44',  // gold    — system body
} as const

/**
 * The key used by the frontend when the user sends a chat message:
 *   window.mp.send(BROWSER_EVENT_TYPE, text)
 * which becomes: window.skymp.send({ type: BROWSER_EVENT_TYPE, data: text })
 */
export const BROWSER_EVENT_TYPE = 'cef::chat:send'
