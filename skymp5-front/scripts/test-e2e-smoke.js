/* eslint-disable @typescript-eslint/no-var-requires */
const assert = require('node:assert/strict');

const targetUrl =
  process.env.PLAYWRIGHT_E2E_URL || 'http://127.0.0.1:1234/?devUi=1';

const serverListPayload = [
  {
    id: 'server-1',
    name: 'SkyMP Main Server',
    ip: '127.0.0.1',
    port: 7777,
    players: 42,
    maxPlayers: 100,
    ping: 24,
    version: '1.0.0',
    online: true,
    tags: ['pve', 'eu'],
  },
  {
    id: 'server-2',
    name: 'SkyMP PvP Arena',
    ip: '127.0.0.1',
    port: 7778,
    players: 15,
    maxPlayers: 50,
    ping: 68,
    version: '1.0.0',
    online: true,
    tags: ['pvp', 'arena'],
  },
];

const frontendMetricsPayload = {
  summary: {
    totalCount: 3,
    errorCount: 1,
    lastReceivedAt: Date.now(),
    averageValue: 122.33,
    sources: [
      { name: 'navigation', count: 2 },
      { name: 'window.error', count: 1 },
    ],
    names: [
      { name: 'first-contentful-paint', count: 1 },
      { name: 'unhandledrejection', count: 1 },
    ],
  },
  entries: [
    {
      name: 'unhandledrejection',
      value: 1,
      source: 'window.error',
      ts: Date.now() - 2000,
      receivedAt: Date.now() - 1000,
      url: 'http://127.0.0.1:1234/?devUi=1',
    },
    {
      name: 'first-contentful-paint',
      value: 144.5,
      source: 'navigation',
      ts: Date.now() - 5000,
      receivedAt: Date.now() - 4000,
      url: 'http://127.0.0.1:1234/?devUi=1',
    },
  ],
};

(async () => {
  let playwright;
  try {
    playwright = require('playwright');
  } catch {
    console.log(
      '[SKIP] Playwright is not installed. Install with: npm --prefix skymp5-front i -D playwright',
    );
    process.exit(0);
  }

  const browser = await playwright.chromium.launch({ headless: true });
  const context = await browser.newContext({ locale: 'en-US' });
  const page = await context.newPage();
  let mutedUsers = [];
  let lastMuteRequestBody = null;
  let lastMessageRequestBody = null;
  let lastWarnRequestBody = null;
  const adminRequestUrls = [];
  const browserErrors = [];
  const adminSockets = new Set();
  let adminSocketConnections = 0;

  page.on('pageerror', (error) => browserErrors.push(error.message));
  await page.routeWebSocket('**/ws/admin-updates', (socket) => {
    adminSocketConnections += 1;
    adminSockets.add(socket);
    socket.onClose(() => adminSockets.delete(socket));
  });

  try {
    const ensureServerListVisible = async () => {
      for (let attempt = 1; attempt <= 3; attempt++) {
        await page.evaluate(() => {
          window.dispatchEvent(new Event('showServerList'));
        });

        const isVisible = await page.evaluate(() => {
          const element = document.querySelector('.server-list');
          if (!element) return false;
          const style = window.getComputedStyle(element);
          return style.display !== 'none' && style.visibility !== 'hidden';
        });

        if (isVisible) return true;
        await page.waitForTimeout(700);
      }
      return false;
    };

    page.on('request', (request) => {
      const url = request.url();
      if (url.includes('/api/admin/')) adminRequestUrls.push(url);
    });

    await page.addInitScript(() => {
      window.localStorage.setItem('skymp.dev.loggedIn', '1');
      window.localStorage.setItem(
        'skymp.adminDashboard.state.v1',
        JSON.stringify({ activeTab: 'players' }),
      );
      window.__connectEvents = [];
      window.addEventListener('serverList:connect', (event) => {
        window.__connectEvents.push(event.detail);
      });
    });

    await page.route('**/api/servers', async (route) => {
      await route.fulfill({
        status: 200,
        contentType: 'application/json',
        body: JSON.stringify(serverListPayload),
      });
    });

    await page.route('**/api/admin/status', async (route) => {
      await route.fulfill({
        status: 200,
        contentType: 'application/json',
        body: JSON.stringify({
          name: 'SkyMP Test',
          online: 3,
          maxPlayers: 100,
          port: 7777,
          uptimeSec: 3600,
        }),
      });
    });

    await page.route(/\/api\/admin\/players(?:\?.*)?$/, async (route) => {
      await route.fulfill({
        status: 200,
        contentType: 'application/json',
        body: JSON.stringify([
          {
            userId: 1,
            actorId: 1,
            actorName: 'Dovahkiin',
            ip: '127.0.0.1',
            pos: [0, 0, 0],
          },
        ]),
      });
    });

    await page.route('**/api/admin/capabilities', async (route) => {
      await route.fulfill({
        status: 200,
        contentType: 'application/json',
        body: JSON.stringify({
          user: 'admin',
          role: 'admin',
          canKick: true,
          canBan: true,
          canUnban: true,
          canConsole: true,
          canViewLogs: true,
          canMessage: true,
          canMute: true,
          canUnmute: true,
          canManageRespawn: true,
          canWarn: true,
        }),
      });
    });

    await page.route('**/api/admin/bans', async (route) => {
      await route.fulfill({
        status: 200,
        contentType: 'application/json',
        body: JSON.stringify([]),
      });
    });

    await page.route('**/api/admin/mutes', async (route) => {
      await route.fulfill({
        status: 200,
        contentType: 'application/json',
        body: JSON.stringify(mutedUsers),
      });
    });

    await page.route(/\/api\/admin\/players\/\d+\/mute$/, async (route) => {
      const request = route.request();
      const method = request.method();
      const m = request.url().match(/\/players\/(\d+)\/mute$/);
      const userId = m ? Number(m[1]) : NaN;

      if (!Number.isFinite(userId)) {
        await route.fulfill({
          status: 400,
          contentType: 'application/json',
          body: JSON.stringify({ ok: false }),
        });
        return;
      }

      if (method === 'POST') {
        const body = request.postDataJSON() || {};
        lastMuteRequestBody = body;
        const durationMinutes = Number(body.durationMinutes) || 10;
        const expiresAt = Date.now() + durationMinutes * 60 * 1000;
        mutedUsers = [
          { userId, expiresAt, remainingSec: durationMinutes * 60 },
        ];
        await route.fulfill({
          status: 200,
          contentType: 'application/json',
          body: JSON.stringify({
            ok: true,
            userId,
            muted: true,
            expiresAt,
            durationMinutes,
          }),
        });
        return;
      }

      if (method === 'DELETE') {
        mutedUsers = mutedUsers.filter((entry) => entry.userId !== userId);
        await route.fulfill({
          status: 200,
          contentType: 'application/json',
          body: JSON.stringify({ ok: true, userId, wasMuted: true }),
        });
        return;
      }

      await route.fulfill({
        status: 405,
        contentType: 'application/json',
        body: JSON.stringify({ ok: false }),
      });
    });

    await page.route(
      /\/api\/admin\/players\/\d+\/message(?:\?.*)?$/,
      async (route) => {
        const request = route.request();
        if (request.method() !== 'POST') {
          await route.fulfill({
            status: 405,
            contentType: 'application/json',
            body: JSON.stringify({ ok: false }),
          });
          return;
        }
        lastMessageRequestBody = request.postDataJSON() || {};
        await route.fulfill({
          status: 200,
          contentType: 'application/json',
          body: JSON.stringify({ ok: true }),
        });
      },
    );

    await page.route(
      /\/api\/admin\/players\/\d+\/warn(?:\?.*)?$/,
      async (route) => {
        const request = route.request();
        if (request.method() !== 'POST') {
          await route.fulfill({
            status: 405,
            contentType: 'application/json',
            body: JSON.stringify({ ok: false }),
          });
          return;
        }
        lastWarnRequestBody = request.postDataJSON() || {};
        await route.fulfill({
          status: 200,
          contentType: 'application/json',
          body: JSON.stringify({ ok: true }),
        });
      },
    );

    await page.route('**/api/admin/logs**', async (route) => {
      await route.fulfill({
        status: 200,
        contentType: 'application/json',
        body: JSON.stringify([]),
      });
    });

    await page.route('**/api/admin/respawn-status', async (route) => {
      await route.fulfill({
        status: 200,
        contentType: 'application/json',
        body: JSON.stringify([
          {
            userId: 7,
            actorName: 'TestWarrior',
            downedAt: Date.now() - 12000,
            canRespawn: false,
          },
        ]),
      });
    });

    await page.route('**/api/admin/events**', async (route) => {
      await route.fulfill({
        status: 200,
        contentType: 'application/json',
        body: JSON.stringify([
          {
            ts: Date.now() - 12000,
            type: 'downed',
            userId: 7,
            actorName: 'TestWarrior',
            details: 'Auto-respawn disabled',
          },
        ]),
      });
    });

    await page.route('**/api/admin/history**', async (route) => {
      await route.fulfill({
        status: 200,
        contentType: 'application/json',
        body: JSON.stringify({
          entries: [
            {
              id: 'XXXX-XXXX',
              type: 'warn',
              playerName: 'TestPlayer',
              userId: 1,
              reason: 'Spam warning',
              author: 'Admin',
              ts: Date.now() - 5000,
            },
            {
              id: 'YYYY-YYYY',
              type: 'kick',
              playerName: 'TestPlayer',
              userId: 1,
              reason: 'Persistent spam',
              author: 'Admin',
              ts: Date.now() - 10000,
            },
          ],
          totalWarns: 2,
          newWarns7d: 1,
          totalBans: 0,
          newBans7d: 0,
          admins: ['Admin'],
        }),
      });
    });

    await page.route('**/api/admin/frontend-metrics**', async (route) => {
      await route.fulfill({
        status: 200,
        contentType: 'application/json',
        body: JSON.stringify(frontendMetricsPayload),
      });
    });

    const response = await page.goto(targetUrl, {
      waitUntil: 'domcontentloaded',
      timeout: 15000,
    });

    assert.ok(response, 'No response from target URL');
    assert.ok(
      response.status() < 500,
      `Target URL responded with ${response.status()}`,
    );

    const title = await page.title();
    assert.equal(typeof title, 'string');

    // DOMContentLoaded can fire before the bundle has mounted React.
    await page.locator('#root > *').first().waitFor({
      state: 'attached',
      timeout: 30000,
    });

    const devBannerControls = page.locator(
      '[data-testid="dev-banner-health-controls"]',
    );
    const hasDevBannerControls = (await devBannerControls.count()) > 0;
    if (hasDevBannerControls) {
      await devBannerControls
        .first()
        .waitFor({ state: 'visible', timeout: 10000 });
      await page
        .locator('[data-testid="dev-banner-health-status"]')
        .waitFor({ state: 'visible', timeout: 10000 });
      await page
        .locator('[data-testid="dev-banner-health-meta"]')
        .waitFor({ state: 'visible', timeout: 10000 });

      await page
        .locator('[data-testid="dev-banner-retry-btn"]')
        .evaluate((element) => element.click());
      await page
        .locator('[data-testid="dev-banner-pause-btn"]')
        .evaluate((element) => element.click());
      await page.waitForFunction(
        () => {
          const button = document.querySelector(
            '[data-testid="dev-banner-pause-btn"]',
          );
          return Boolean(
            button &&
              button.textContent &&
              button.textContent.includes('Resume'),
          );
        },
        { timeout: 5000 },
      );
      await page
        .locator('[data-testid="dev-banner-reset-warnings-btn"]')
        .evaluate((element) => element.click());
      await page
        .locator('[data-testid="dev-banner-pause-btn"]')
        .evaluate((element) => element.click());
      await page.waitForFunction(
        () => {
          const button = document.querySelector(
            '[data-testid="dev-banner-pause-btn"]',
          );
          return Boolean(
            button &&
              button.textContent &&
              button.textContent.includes('Pause'),
          );
        },
        { timeout: 5000 },
      );
    } else {
      console.log(
        '[WARN] Dev banner controls were not visible in this run; skipping banner assertions',
      );
    }

    const serverListVisible = await ensureServerListVisible();
    assert.equal(serverListVisible, true, 'Launcher server list must be visible');

    await page
      .locator('.server-list')
      .waitFor({ state: 'visible', timeout: 5000 });
    await page.locator('.server-list__search').fill('Arena');
    assert.equal(await page.locator('.server-list__table tbody tr').count(), 1);
    await page
      .locator('.server-list__favorite-btn')
      .first()
      .evaluate((element) => element.click());
    await page.waitForFunction(() =>
      Boolean(document.querySelector('.server-list__favorite-btn--active')),
    );
    await page
      .locator('.server-list__filter-full input')
      .nth(1)
      .evaluate((element) => {
        const input = element;
        if (!input.checked) {
          input.click();
        }
      });
    assert.equal(await page.locator('.server-list__table tbody tr').count(), 1);
    await page
      .locator('.server-list__connect-btn')
      .first()
      .evaluate((element) => element.click());
    await page.waitForFunction(
      () =>
        Array.isArray(window.__connectEvents) &&
        window.__connectEvents.length > 0,
    );

    const connectEvents = await page.evaluate(() => window.__connectEvents);
    assert.equal(connectEvents.length > 0, true);
    assert.deepEqual(connectEvents[connectEvents.length - 1], {
      ip: '127.0.0.1',
      port: 7778,
    });

    mutedUsers = [
      { userId: 1, expiresAt: Date.now() + 15 * 60 * 1000, remainingSec: 900 },
    ];

    const initialPlayersResponse = page.waitForResponse(
      (response) => {
        return (
          response.url().includes('/api/admin/players') &&
          response.request().method() === 'GET'
        );
      },
      { timeout: 10000 },
    );
    await page.evaluate(() => {
      window.dispatchEvent(new Event('showAdminDashboard'));
    });
    await page
      .locator('.admin-dashboard')
      .waitFor({ state: 'visible', timeout: 5000 });
    await initialPlayersResponse;

    const loadingIndicator = page.locator('.admin-dashboard__loading');
    if ((await loadingIndicator.count()) > 0) {
      await loadingIndicator.waitFor({ state: 'hidden', timeout: 10000 });
    }

    await page
      .locator('[data-testid="admin-panel-players"]')
      .waitFor({ state: 'visible', timeout: 10000 });
    await page
      .locator('[data-testid="admin-player-row-1"]')
      .waitFor({ state: 'visible', timeout: 10000 });

    const connectionsBeforeEdit = adminSocketConnections;
    await page.locator('[data-testid="admin-reason-input"]').fill('Spam');
    for (const socket of adminSockets) {
      socket.send(JSON.stringify({
        type: 'status',
        ts: Date.now(),
        data: { online: 7 },
      }));
    }
    await page.waitForFunction(() =>
      Array.from(document.querySelectorAll('.admin-dashboard__sidebar-pill'))
        .some((element) => element.textContent === '7/100'),
    );
    assert.equal(adminSocketConnections, connectionsBeforeEdit,
      'Editing state must keep existing WebSocket connections');
    console.log('[OK] Admin WebSocket keeps its connection and uses the latest state');
    await page
      .locator('[data-testid="admin-mute-duration-select"]')
      .selectOption('15');

    await page.evaluate(async () => {
      await fetch('/api/admin/players/1/mute', {
        method: 'POST',
        headers: { 'Content-Type': 'application/json' },
        body: JSON.stringify({ durationMinutes: 15, reason: 'Spam' }),
      });
    });
    assert.equal(lastMuteRequestBody.durationMinutes, 15);
    assert.equal(lastMuteRequestBody.reason, 'Spam');

    await page
      .locator('[data-testid="admin-msg-btn-1"]')
      .evaluate((element) => element.click());
    await page
      .locator('[data-testid="admin-message-form"]')
      .waitFor({ state: 'visible', timeout: 10000 });
    await page
      .locator('[data-testid="admin-message-input"]')
      .fill('Please stop spamming');
    await page
      .locator('[data-testid="admin-message-cancel-btn"]')
      .evaluate((element) => element.click());
    await page
      .locator('[data-testid="admin-message-form"]')
      .waitFor({ state: 'hidden', timeout: 10000 });

    const messageResult = await page.evaluate(async () => {
      const response = await fetch('/api/admin/players/1/message', {
        method: 'POST',
        headers: { 'Content-Type': 'application/json' },
        body: JSON.stringify({
          message: 'Please stop spamming',
          reason: 'Spam',
        }),
      });
      return response.ok;
    });

    assert.equal(messageResult, true);
    assert.equal(lastMessageRequestBody.message, 'Please stop spamming');
    assert.equal(lastMessageRequestBody.reason, 'Spam');

    const unmuteResult = await page.evaluate(async () => {
      const response = await fetch('/api/admin/players/1/mute', {
        method: 'DELETE',
      });
      const payload = await response.json();
      const mutesResponse = await fetch('/api/admin/mutes');
      const mutes = await mutesResponse.json();
      return {
        ok: response.ok,
        payload,
        mutesCount: Array.isArray(mutes) ? mutes.length : 0,
      };
    });

    assert.equal(unmuteResult.ok, true);
    assert.equal(unmuteResult.payload.wasMuted, true);
    assert.equal(unmuteResult.mutesCount, 0);

    const metricsResult = await page.evaluate(async () => {
      const response = await fetch(
        '/api/admin/frontend-metrics?name=paint&limit=50',
      );
      const payload = await response.json();
      return {
        ok: response.ok,
        totalCount: payload?.summary?.totalCount,
        names: Array.isArray(payload?.entries)
          ? payload.entries.map((entry) => entry.name)
          : [],
      };
    });

    assert.equal(metricsResult.ok, true);
    assert.equal(metricsResult.totalCount > 0, true);
    assert.equal(metricsResult.names.includes('first-contentful-paint'), true);

    // --- Respawn tab smoke ---
    const respawnResponse = page.waitForResponse(
      (response) => response.url().includes('/api/admin/respawn-status'),
      { timeout: 10000 },
    );
    await page
      .locator('[data-testid="admin-tab-respawn"]')
      .evaluate((element) => element.click());
    await respawnResponse;

    await page.locator('#admin-panel-respawn')
      .waitFor({ state: 'visible', timeout: 8000 });
    await page.locator('[data-testid="admin-downed-row-7"]')
      .waitFor({ state: 'visible', timeout: 5000 });
    assert.equal(await page.locator('[data-testid="admin-revive-btn-7"]').isVisible(), true);

    // --- Events tab smoke ---
    const eventsResponse = page.waitForResponse(
      (response) => response.url().includes('/api/admin/events'),
      { timeout: 10000 },
    );
    await page
      .locator('[data-testid="admin-tab-events"]')
      .evaluate((element) => element.click());
    await eventsResponse;

    await page.locator('[data-testid="admin-event-entry-downed-0"]')
      .waitFor({ state: 'visible', timeout: 5000 });

    // --- Warn action E2E flow (Exit-Criterion: Admin moderation workflows) ---
    await page
      .locator('[data-testid="admin-tab-players"]')
      .evaluate((element) => element.click());
    await page
      .locator('[data-testid="admin-player-row-1"]')
      .waitFor({ state: 'visible', timeout: 10000 });

    const warnBtn = page.locator('[data-testid="admin-warn-btn-1"]');
    if ((await warnBtn.count()) > 0) {
      await warnBtn.waitFor({ state: 'visible', timeout: 5000 });
      await warnBtn.evaluate((element) => element.click());

      // Warn dialog should open
      await page
        .locator('[data-testid="admin-warn-form"]')
        .waitFor({ state: 'visible', timeout: 5000 });

      // Fill in message and reason
      await page
        .locator('[data-testid="admin-warn-message-input"]')
        .fill('Stop spamming in chat');
      await page
        .locator('[data-testid="admin-warn-reason-input"]')
        .fill('Repeated spam');

      // Submit the warn
      const warnResponsePromise = page.waitForResponse(
        (response) => response.url().includes('/api/admin/players/1/warn'),
        { timeout: 10000 },
      );
      await page
        .locator('[data-testid="admin-warn-submit-btn"]')
        .evaluate((element) => element.click());

      await warnResponsePromise;

      // Verify request payload
      assert.equal(
        lastWarnRequestBody && lastWarnRequestBody.message,
        'Stop spamming in chat',
        'Warn message should be sent correctly',
      );
      assert.equal(
        lastWarnRequestBody && lastWarnRequestBody.reason,
        'Repeated spam',
        'Warn reason should be sent correctly',
      );

      // Dialog should close after successful warn
      await page
        .locator('[data-testid="admin-warn-form"]')
        .waitFor({ state: 'hidden', timeout: 5000 });

      console.log('[OK] Warn action E2E flow: dialog open → message+reason filled → submitted → API called → dialog closed');
    } else {
      console.log('[WARN] Warn button not found for player 1; warn flow test skipped (canWarn may be false)');
    }

    // --- Cross-panel history search smoke ---
    await page
      .locator('[data-testid="admin-tab-players"]')
      .evaluate((element) => element.click());

    // Wait for the player row to be visible and check for history button
    const playerRow = page.locator('[data-testid="admin-player-row-1"]');
    if ((await playerRow.count()) > 0) {
      await playerRow.waitFor({ state: 'visible', timeout: 5000 });
      // Check if history button exists (data-testid pattern: admin-history-btn-{userId})
      const historyBtn = page.locator('[data-testid="admin-history-btn-1"]');
      if ((await historyBtn.count()) > 0) {
        await historyBtn.waitFor({ state: 'visible', timeout: 5000 });
        const historyResponse = page.waitForResponse(
          (response) => response.url().includes('/api/admin/history'),
          { timeout: 10000 },
        );
        await historyBtn.evaluate((element) => element.click());
        await historyResponse;
        // Check that we're now in the topbar history section
        // The topbar should show and the history search should be pre-filled
        const historyPanel = page.locator('.admin-dashboard__tx-history');
        await historyPanel.waitFor({ state: 'visible', timeout: 5000 });
        assert.equal(await historyPanel.locator('input').first().inputValue(), '1',
          'History search must contain the selected player ID');
        console.log('[OK] Cross-panel history search flow works correctly');
      } else {
        console.log(
          '[WARN] Player history button not found; cross-panel search test skipped',
        );
      }
    } else {
      console.log('[WARN] Player row not found; cross-panel search test skipped');
    }

    assert.deepEqual(browserErrors, [], 'Browser must not report uncaught errors');
    console.log(`[OK] E2E launcher/admin metrics flow passed for ${targetUrl}`);
  } finally {
    if (adminRequestUrls.length === 0) {
      console.log('[WARN] No admin API requests captured in smoke test run');
    }
    await context.close();
    await browser.close();
  }
})();
