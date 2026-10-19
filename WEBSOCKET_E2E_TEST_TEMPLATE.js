/**
 * E2E Test Template for WebSocket Real-Time Updates
 * Add this to scripts/test-e2e-smoke.js after cross-panel history tests
 *
 * Status: ⏳ Pending integration (2026-05-17)
 * - Backend WS server and frontend hook are complete.
 * - This template has not yet been merged into scripts/test-e2e-smoke.js.
 * - TODO: integrate after manual integration test confirms WS in local dev env.
 */

// --- WebSocket real-time updates smoke ---
console.log('[START] WebSocket real-time updates test');

const wsConnectionTest = async () => {
  return new Promise((resolve) => {
    const ws = new WebSocket(
      `ws://${window.location.hostname}:${window.location.port}/ws/admin-updates`,
    );

    let messageCount = 0;
    let statusReceived = false;
    let playersReceived = false;
    const timeout = setTimeout(() => {
      ws.close();
      resolve({
        connected: false,
        messageCount,
        statusReceived,
        playersReceived,
        error: 'timeout',
      });
    }, 5000);

    ws.onopen = () => {
      console.log('[OK] WebSocket connected to /ws/admin-updates');
    };

    ws.onmessage = (event) => {
      messageCount++;
      try {
        const message = JSON.parse(event.data);

        if (message.type === 'status') {
          statusReceived = true;
          console.log('[OK] Received status update:', {
            online: message.data.online,
            maxPlayers: message.data.maxPlayers,
          });
        }

        if (message.type === 'players') {
          playersReceived = true;
          console.log('[OK] Received players update:', {
            playerCount: message.data.players?.length || 0,
          });
        }

        // Check if we got at least one of each important message
        if (messageCount >= 3 && (statusReceived || playersReceived)) {
          clearTimeout(timeout);
          ws.close();
          resolve({
            connected: true,
            messageCount,
            statusReceived,
            playersReceived,
            error: null,
          });
        }
      } catch (error) {
        console.error('[ERROR] Failed to parse WebSocket message:', error);
      }
    };

    ws.onerror = () => {
      clearTimeout(timeout);
      ws.close();
      resolve({
        connected: false,
        messageCount,
        statusReceived,
        playersReceived,
        error: 'connection error',
      });
    };
  });
};

const wsResult = await page.evaluate(wsConnectionTest);

// WebSocket connection test assertions
assert.equal(wsResult.connected, true, 'WebSocket should connect to /ws/admin-updates');
assert.equal(wsResult.messageCount >= 3, true, 'Should receive at least 3 messages');
assert.equal(
  wsResult.statusReceived || wsResult.playersReceived,
  true,
  'Should receive status or players update',
);
console.log(`[OK] WebSocket real-time updates test passed (${wsResult.messageCount} messages)`);

// --- WebSocket reconnection test ---
const wsReconnectTest = async () => {
  return new Promise((resolve) => {
    const ws = new WebSocket(
      `ws://${window.location.hostname}:${window.location.port}/ws/admin-updates`,
    );

    let connectedOnce = false;
    const disconnectTimeout = setTimeout(() => {
      ws.close();
    }, 1000);

    ws.onopen = () => {
      connectedOnce = true;
      clearTimeout(disconnectTimeout);
      // Manually close to test reconnect
      ws.close();

      // Now check if we can reconnect
      setTimeout(() => {
        const ws2 = new WebSocket(
          `ws://${window.location.hostname}:${window.location.port}/ws/admin-updates`,
        );
        let reconnected = false;
        const timeout = setTimeout(() => {
          ws2.close();
          resolve({
            initialConnection: connectedOnce,
            reconnected,
            error: 'reconnect timeout',
          });
        }, 2000);

        ws2.onopen = () => {
          reconnected = true;
          clearTimeout(timeout);
          ws2.close();
          resolve({
            initialConnection: connectedOnce,
            reconnected,
            error: null,
          });
        };

        ws2.onerror = () => {
          clearTimeout(timeout);
          resolve({
            initialConnection: connectedOnce,
            reconnected: false,
            error: 'reconnect failed',
          });
        };
      }, 100);
    };

    ws.onerror = () => {
      resolve({
        initialConnection: false,
        reconnected: false,
        error: 'initial connection failed',
      });
    };
  });
};

const wsReconnectResult = await page.evaluate(wsReconnectTest);
assert.equal(wsReconnectResult.initialConnection, true, 'Initial WebSocket connection should succeed');
assert.equal(wsReconnectResult.reconnected, true, 'WebSocket should be able to reconnect');
console.log('[OK] WebSocket reconnection test passed');

// --- WebSocket fallback to polling test ---
console.log('[INFO] WebSocket fallback to polling test requires manual HTTP endpoint verification');
console.log('[INFO] Verify that if WebSocket fails, admin dashboard still functions with polling');

console.log(`[OK] WebSocket real-time updates smoke tests completed`);

/**
 * Integration into test-e2e-smoke.js
 *
 * Location: After the "Cross-panel history search smoke" section (around line 673)
 *
 * Before the final:
 *   console.log(`[OK] E2E launcher/admin metrics flow passed for ${targetUrl}`);
 *   await context.close();
 *   await browser.close();
 *
 * Add:
 *   // --- WebSocket real-time updates smoke ---
 *   // [INSERT CODE ABOVE]
 *
 * Notes:
 * - This test runs inside page.evaluate() context (browser sandbox)
 * - WebSocket must be available on same host/port as the page
 * - Expected messages: 'connected', 'status', 'players' (repeating)
 * - Timeout is 5 seconds per test
 * - No special mocking needed (tests against real WebSocket server)
 */
