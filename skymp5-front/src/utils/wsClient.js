// ── WS Relay Client (browser) ─────────────────────────────────────────────────
//
// Connects skymp5-front (running inside SkyMP's CEF browser) to the
// Frostfall-Backend WS relay.
//
// Auth flow:
//   1. Server writes window.ffWsNonce via executeJavaScript (ff_wsNonce property)
//   2. Server calls window.ffWsConnect() once the nonce is ready
//   3. This client sends { type:'auth', nonce } to the relay
//   4. Relay confirms with { type:'auth_ok', userId }
//
// Incoming chat_msg payloads are pushed into the existing window._ffChatPush
// pipeline (same as the mp.set path) — no React changes needed.
//
// Outgoing chat: skyrimPlatform.sendMessage('cef::chat:send', text) is
// intercepted and sent over WS instead when authenticated. The SP-runtime
// path is left intact as a fallback for the unauthenticated window.
//
// WS URL: window.ffWsUrl if set by the server, otherwise ws://localhost:7778.
// The server can inject this via executeJavaScript before the page loads so
// remote clients reach the correct host.

(function () {
  'use strict';

  const WS_URL = (typeof window.ffWsUrl === 'string' && window.ffWsUrl)
    ? window.ffWsUrl
    : 'ws://localhost:7778';

  let ws            = null;
  let authenticated = false;

  // ── Connection ──────────────────────────────────────────────────────────────

  function connect() {
    try {
      ws = new WebSocket(WS_URL);
    } catch (e) {
      console.warn('[ws-client] failed to open WebSocket:', e);
      setTimeout(connect, 5000);
      return;
    }

    ws.onopen = function () {
      // Authenticate immediately if the nonce is already available
      if (window.ffWsNonce) {
        authenticate();
      }
      // Otherwise ffWsConnect() will be called by the server once the nonce arrives
    };

    ws.onmessage = function (event) {
      var msg;
      try { msg = JSON.parse(event.data); } catch { return; }

      if (msg.type === 'auth_ok') {
        authenticated = true;
        console.log('[ws-client] authenticated, userId=' + msg.userId);
        return;
      }

      if (msg.type === 'auth_fail') {
        console.warn('[ws-client] auth failed:', msg.reason);
        return;
      }

      if (msg.type === 'chat_msg') {
        pushChatMsg(msg.msg);
      }
    };

    ws.onclose = function () {
      authenticated = false;
      ws = null;
      setTimeout(connect, 3000);
    };

    ws.onerror = function (e) {
      console.warn('[ws-client] socket error', e);
    };
  }

  function authenticate() {
    if (ws && ws.readyState === WebSocket.OPEN && window.ffWsNonce) {
      ws.send(JSON.stringify({ type: 'auth', nonce: window.ffWsNonce }));
    }
  }

  // ── Incoming chat ───────────────────────────────────────────────────────────

  function pushChatMsg(msg) {
    if (typeof window._ffChatPush === 'function') {
      window._ffChatPush(msg);
    } else {
      // _ffChatPush not yet defined (race at session start) — queue it
      if (!Array.isArray(window._ffChatPendingMsgs)) window._ffChatPendingMsgs = [];
      window._ffChatPendingMsgs.push(msg);
    }
  }

  // ── Outgoing chat interception ──────────────────────────────────────────────
  //
  // Wraps skyrimPlatform.sendMessage so that 'cef::chat:send' goes over WS
  // when we're authenticated, leaving all other messages on the SP path.
  // Called once on load — if skyrimPlatform isn't available yet (dev browser)
  // the original fallback (no-op) still works.

  function patchSkyrimPlatform() {
    var sp = window.skyrimPlatform;
    if (!sp || typeof sp.sendMessage !== 'function') return;

    var original = sp.sendMessage.bind(sp);
    sp.sendMessage = function (type) {
      if (type === 'cef::chat:send' && authenticated && ws && ws.readyState === WebSocket.OPEN) {
        ws.send(JSON.stringify({ type: 'chat_send', text: arguments[1] }));
        return;
      }
      original.apply(sp, arguments);
    };
  }

  // ── Server-called hook ──────────────────────────────────────────────────────
  //
  // The server writes window.ffWsNonce via executeJavaScript then immediately
  // calls window.ffWsConnect(). If the socket is already open we auth now;
  // if it's still connecting, onopen will call authenticate() once it opens.

  window.ffWsConnect = function () {
    if (ws && ws.readyState === WebSocket.OPEN) {
      authenticate();
    }
    // else: onopen handler will call authenticate() once the socket opens
  };

  // ── Boot ────────────────────────────────────────────────────────────────────

  patchSkyrimPlatform();
  connect();

}());
