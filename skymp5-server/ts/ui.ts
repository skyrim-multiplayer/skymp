const Koa = require("koa");
const serve = require("koa-static");
const proxy = require("koa-proxy");
const Router = require("koa-router");
const auth = require("koa-basic-auth");
import * as koaBody from "koa-body";
import * as http from "http";
import { Settings } from "./settings";
import Axios from "axios";
import { AddressInfo } from "net";
import { register, getAggregatedMetrics, rpcCallsCounter, rpcDurationHistogram } from "./systems/metricsSystem";

let gScampServer: any = null;

let metricsAuth: { user: string; password: string } | null = null;
let adminAuth: { user: string; password: string } | null = null;
const DISABLE_AUTH_PASSWORD = "I know what I'm doing, disable metrics auth";
const connectedUsers = new Map<number, { connectedAt: number }>();

const metricsAuthParse = (settings: Settings): void => {
  const authConfig = settings.allSettings?.metricsAuth as { user?: string; password?: string } | undefined;
  if (!authConfig) {
    console.log('Metrics auth is not configured, so it will be inaccessible. Set metricsAuth setting to activate');
    return;
  }
  if (!authConfig.user || !authConfig.password) {
    console.error('metricsAuth setting must contain user and password fields');
    return;
  }
  metricsAuth = { user: authConfig.user, password: authConfig.password };
}

const adminAuthParse = (settings: Settings): void => {
  const authConfig = settings.allSettings?.adminAuth as { user?: string; password?: string } | undefined;
  if (!authConfig) {
    console.log('Admin dashboard auth is not configured; /admin will only accept loopback requests');
    return;
  }
  if (!authConfig.user || !authConfig.password) {
    console.error('adminAuth setting must contain user and password fields');
    return;
  }
  adminAuth = { user: authConfig.user, password: authConfig.password };
}

const isLoopbackIp = (ip: string | undefined): boolean => {
  if (!ip) return false;
  return ip === "127.0.0.1" || ip === "::1" || ip === "::ffff:127.0.0.1" || ip === "localhost";
}

const adminGuard = async (ctx: any, next: any) => {
  if (adminAuth) {
    if (adminAuth.password === DISABLE_AUTH_PASSWORD) {
      return next();
    }
    return auth({ name: adminAuth.user, pass: adminAuth.password })(ctx, next);
  }
  if (!isLoopbackIp(ctx.request.ip)) {
    ctx.status = 403;
    ctx.body = { ok: false, error: "Admin dashboard requires adminAuth for non-local requests" };
    return;
  }
  return next();
}

const getAdminApi = (): any | null => {
  const api = (globalThis as any).frostfallAdminDashboard;
  return api && typeof api === "object" ? api : null;
}

const runAdminApi = (method: string, payload?: unknown): { status: number; body: unknown } => {
  const api = getAdminApi();
  if (!api || typeof api[method] !== "function") {
    if (method === "getSnapshot") {
      return {
        status: 200,
        body: {
          ok: true,
          data: {
            players: getNativeConnectedPlayerSnapshots(),
            editableFields: [],
            knownActorProperties: [],
            adminApiReady: false,
          },
        },
      };
    }
    return { status: 503, body: { ok: false, error: "Gamemode admin API is not ready yet" } };
  }
  try {
    const result = api[method](payload);
    if (result?.ok === false) {
      const playerId = Number((payload as any)?.playerId);
      if (Number.isInteger(playerId) && connectedUsers.has(playerId) && String(result.error).includes("not connected")) {
        result.error = "Player is connected, but gamemode state is not ready yet";
      }
    }
    if (method === "getSnapshot" && result?.ok && result.data) {
      result.data.players = mergeNativeConnectedPlayers(result.data.players);
      result.data.adminApiReady = true;
    }
    return { status: result?.ok === false ? 400 : 200, body: result };
  } catch (err: any) {
    return { status: 500, body: { ok: false, error: String(err?.message ?? err) } };
  }
}

const mergeNativeConnectedPlayers = (players: any[]): any[] => {
  const result = Array.isArray(players) ? [...players] : [];
  const nativeById = new Map(getNativeConnectedPlayerSnapshots().map((player: any) => [player.id, player]));
  const seen = new Set<number>();
  for (const player of result) {
    const id = Number(player?.id);
    seen.add(id);
    const native = nativeById.get(id);
    if (native) {
      player.connected = true;
      player.guid = player.guid ?? native.guid;
      player.ip = player.ip ?? native.ip;
      player.pos = player.pos ?? native.pos;
      player.cellOrWorld = player.cellOrWorld ?? native.cellOrWorld;
      player.pendingGamemode = false;
    }
  }
  for (const player of getNativeConnectedPlayerSnapshots()) {
    if (!seen.has(player.id)) {
      result.push(player);
    }
  }
  return result.sort((a: any, b: any) => Number(a.id) - Number(b.id));
}

const getNativeConnectedPlayerSnapshots = (): any[] => {
  const snapshots: any[] = [];
  for (const [userId, entry] of connectedUsers) {
    const actorId = safeScampCall(() => gScampServer?.getUserActor(userId), 0) || 0;
    snapshots.push({
      id: userId,
      actorId,
      name: safeScampCall(() => actorId ? gScampServer?.getActorName(actorId) : null, null) || `User${userId}`,
      holdId: null,
      factions: [],
      bounty: {},
      isDown: false,
      isCaptive: false,
      downedAt: null,
      captiveAt: null,
      properties: [],
      hungerLevel: 10,
      drunkLevel: 0,
      septims: 0,
      stipendPaidHours: 0,
      minutesOnline: Math.max(0, Math.floor((Date.now() - entry.connectedAt) / 60000)),
      isStaff: false,
      isLeader: false,
      guid: safeScampCall(() => gScampServer?.getUserGuid(userId), null),
      ip: safeScampCall(() => gScampServer?.getUserIp(userId), null),
      connected: true,
      pos: safeScampCall(() => actorId ? gScampServer?.getActorPos(actorId) : null, null),
      cellOrWorld: safeScampCall(() => actorId ? gScampServer?.getActorCellOrWorld(actorId) : null, null),
      memberships: [],
      actorProperties: {},
      pendingGamemode: true,
    });
  }
  return snapshots;
}

const safeScampCall = <T>(fn: () => T, fallback: T): T => {
  try {
    const value = fn();
    return value === undefined ? fallback : value;
  } catch {
    return fallback;
  }
}

const adminDashboardHtml = `<!doctype html>
<html lang="en">
<head>
  <meta charset="utf-8" />
  <meta name="viewport" content="width=device-width, initial-scale=1" />
  <title>SkyMP Admin</title>
  <style>
    :root {
      color-scheme: light;
      --bg: #f5f6f3;
      --panel: #ffffff;
      --ink: #20231f;
      --muted: #626b61;
      --line: #d9ded5;
      --accent: #2d6f5e;
      --accent-strong: #184c40;
      --bad: #9b2d2d;
      --soft: #e9eee7;
    }
    * { box-sizing: border-box; }
    body {
      margin: 0;
      font-family: Inter, ui-sans-serif, system-ui, -apple-system, BlinkMacSystemFont, "Segoe UI", sans-serif;
      color: var(--ink);
      background: var(--bg);
      letter-spacing: 0;
    }
    header {
      display: flex;
      align-items: center;
      justify-content: space-between;
      gap: 16px;
      padding: 18px 22px;
      border-bottom: 1px solid var(--line);
      background: #fbfcfa;
    }
    h1, h2 { margin: 0; font-weight: 700; }
    h1 { font-size: 20px; }
    h2 { font-size: 16px; }
    main {
      display: grid;
      grid-template-columns: minmax(280px, 380px) minmax(0, 1fr);
      min-height: calc(100vh - 69px);
    }
    aside {
      border-right: 1px solid var(--line);
      background: #fafbf8;
      padding: 16px;
      overflow: auto;
    }
    section { padding: 16px; overflow: auto; }
    .toolbar { display: flex; align-items: center; gap: 10px; flex-wrap: wrap; }
    .status { color: var(--muted); font-size: 13px; }
    .notice {
      display: none;
      margin: 0 0 14px;
      padding: 10px 12px;
      border: 1px solid var(--line);
      background: var(--soft);
      border-radius: 8px;
      font-size: 13px;
    }
    .notice.error { display: block; color: var(--bad); border-color: #e1b9b9; background: #fff4f1; }
    .notice.ok { display: block; color: var(--accent-strong); border-color: #b9d7cd; background: #eef8f4; }
    table { width: 100%; border-collapse: collapse; table-layout: fixed; font-size: 13px; }
    th, td {
      padding: 9px 8px;
      border-bottom: 1px solid var(--line);
      text-align: left;
      vertical-align: middle;
      overflow: hidden;
      text-overflow: ellipsis;
      white-space: nowrap;
    }
    th { color: var(--muted); font-weight: 650; }
    tr.player-row { cursor: pointer; }
    tr.player-row:hover { background: #eef3ee; }
    tr.player-row.selected { background: #e2eee9; }
    .grid { display: grid; grid-template-columns: repeat(2, minmax(220px, 1fr)); gap: 14px; }
    .card { background: var(--panel); border: 1px solid var(--line); border-radius: 8px; padding: 14px; }
    .span { grid-column: 1 / -1; }
    .fields { display: grid; grid-template-columns: repeat(2, minmax(160px, 1fr)); gap: 10px; margin-top: 12px; }
    label { display: grid; gap: 5px; color: var(--muted); font-size: 12px; font-weight: 650; }
    input, textarea, select {
      width: 100%;
      min-height: 36px;
      border: 1px solid var(--line);
      border-radius: 6px;
      background: #fff;
      color: var(--ink);
      padding: 8px 9px;
      font: inherit;
      font-size: 13px;
    }
    textarea { min-height: 96px; resize: vertical; font-family: ui-monospace, SFMono-Regular, Consolas, monospace; }
    .checks { display: grid; grid-template-columns: repeat(2, minmax(120px, 1fr)); gap: 10px; margin-top: 12px; }
    .checks label { display: flex; align-items: center; gap: 8px; color: var(--ink); }
    .checks input { width: 16px; min-height: 16px; }
    button {
      min-height: 36px;
      border: 1px solid var(--accent);
      border-radius: 6px;
      background: var(--accent);
      color: #fff;
      padding: 8px 12px;
      font: inherit;
      font-size: 13px;
      font-weight: 700;
      cursor: pointer;
    }
    button.secondary { background: #fff; color: var(--accent-strong); }
    button.danger { border-color: var(--bad); background: var(--bad); }
    button:disabled { opacity: 0.55; cursor: not-allowed; }
    .actions { display: flex; gap: 8px; flex-wrap: wrap; margin-top: 12px; }
    .kv { display: grid; gap: 8px; margin-top: 12px; font-size: 13px; }
    .kv div { display: flex; justify-content: space-between; gap: 12px; border-bottom: 1px solid var(--line); padding-bottom: 6px; }
    .kv span:first-child { color: var(--muted); }
    .membership { display: grid; grid-template-columns: minmax(120px, 1fr) 90px auto auto; gap: 8px; align-items: end; margin-top: 10px; }
    .empty { color: var(--muted); font-size: 13px; padding: 14px 0; }
    @media (max-width: 920px) {
      main { grid-template-columns: 1fr; }
      aside { border-right: 0; border-bottom: 1px solid var(--line); max-height: 45vh; }
      .grid, .fields { grid-template-columns: 1fr; }
      .membership { grid-template-columns: 1fr 90px; }
    }
  </style>
</head>
<body>
  <header>
    <div>
      <h1>SkyMP Admin</h1>
      <div class="status" id="summary">Loading</div>
    </div>
    <div class="toolbar">
      <button class="secondary" id="refreshBtn">Refresh</button>
      <button class="secondary" id="autoBtn">Auto: on</button>
    </div>
  </header>
  <main>
    <aside>
      <table>
        <thead><tr><th style="width:72px">ID</th><th>Name</th><th style="width:76px">Actor</th></tr></thead>
        <tbody id="players"></tbody>
      </table>
    </aside>
    <section>
      <div id="notice" class="notice"></div>
      <div id="details" class="empty">No connected players.</div>
    </section>
  </main>
  <script>
    const state = { players: [], selectedId: null, auto: true, timer: null, meta: {} };
    const el = (id) => document.getElementById(id);

    function showNotice(text, kind) {
      const node = el('notice');
      node.className = 'notice ' + (kind || '');
      node.textContent = text || '';
    }

    async function request(path, options) {
      const res = await fetch(path, Object.assign({ headers: { 'Content-Type': 'application/json' } }, options || {}));
      const body = await res.json().catch(() => ({ ok: false, error: 'Invalid JSON response' }));
      if (!res.ok || body.ok === false) throw new Error(body.error || ('HTTP ' + res.status));
      return body.data;
    }

    async function refresh(quiet) {
      try {
        const data = await request('/admin/api/snapshot');
        state.meta = data;
        state.players = data.players || [];
        if (state.selectedId === null && state.players.length) state.selectedId = state.players[0].id;
        if (state.selectedId !== null && !state.players.some((p) => p.id === state.selectedId)) {
          state.selectedId = state.players.length ? state.players[0].id : null;
        }
        renderPlayers();
        renderDetails();
        el('summary').textContent = state.players.length + ' connected player' + (state.players.length === 1 ? '' : 's');
        if (!quiet) showNotice('Dashboard refreshed.', 'ok');
      } catch (err) {
        showNotice(err.message, 'error');
      }
    }

    function renderPlayers() {
      el('players').innerHTML = state.players.map((p) =>
        '<tr class="player-row ' + (p.id === state.selectedId ? 'selected' : '') + '" data-id="' + p.id + '">' +
        '<td>' + p.id + '</td><td title="' + esc(p.name) + '">' + esc(p.name) + '</td><td>' + p.actorId + '</td></tr>'
      ).join('');
      document.querySelectorAll('.player-row').forEach((row) => row.addEventListener('click', () => {
        state.selectedId = Number(row.dataset.id);
        renderPlayers();
        renderDetails();
      }));
    }

    function renderDetails() {
      const p = state.players.find((player) => player.id === state.selectedId);
      if (!p) {
        el('details').className = 'empty';
        el('details').textContent = 'No connected players.';
        return;
      }
      el('details').className = 'grid';
      const memberships = p.memberships || [];
      el('details').innerHTML =
        '<div class="card span"><h2>' + esc(p.name) + '</h2><div class="kv">' +
        kv('User ID', p.id) + kv('Actor ID', p.actorId) + kv('GUID', p.guid || 'n/a') +
        kv('IP', p.ip || 'n/a') + kv('Position', p.pos ? p.pos.join(', ') : 'n/a') +
        kv('Cell/World', p.cellOrWorld === null ? 'n/a' : p.cellOrWorld) + '</div></div>' +
        (p.pendingGamemode ? '<div class="notice ok span" style="display:block">Connected at the server layer. Waiting for gamemode player state before admin edits are available.</div>' : '') +
        '<div class="card span"><h2>Attributes</h2>' + attributeForm(p) + '</div>' +
        '<div class="card"><h2>Factions</h2><div id="membershipList">' + membershipHtml(memberships) + '</div>' + factionAddForm() + '</div>' +
        '<div class="card"><h2>Bounty</h2>' + bountyForm() + '<div class="kv">' + Object.keys(p.bounty || {}).map((h) => kv(h, p.bounty[h])).join('') + '</div></div>' +
        '<div class="card"><h2>Actor Property</h2>' + actorPropertyForm() + '</div>' +
        '<div class="card"><h2>Faction Document</h2>' + factionDocForm() + '</div>';
      bindForms();
    }

    function attributeForm(p) {
      return '<div class="fields">' +
        input('name', 'Name', p.name) + input('holdId', 'Hold ID', p.holdId || '') +
        input('hungerLevel', 'Hunger', p.hungerLevel, 'number') + input('drunkLevel', 'Drunk', p.drunkLevel, 'number') +
        input('septims', 'Septims', p.septims, 'number') + input('stipendPaidHours', 'Stipend Hours', p.stipendPaidHours, 'number') +
        input('minutesOnline', 'Minutes Online', p.minutesOnline, 'number') + '</div>' +
        '<div class="checks">' + check('isStaff', 'Staff', p.isStaff) + check('isLeader', 'Leader', p.isLeader) +
        check('isDown', 'Down', p.isDown) + check('isCaptive', 'Captive', p.isCaptive) + '</div>' +
        '<div class="actions"><button id="saveAttrs">Save attributes</button></div>';
    }

    function membershipHtml(memberships) {
      if (!memberships.length) return '<div class="empty">No factions.</div>';
      return memberships.map((m) =>
        '<div class="membership" data-faction="' + escAttr(m.factionId) + '">' +
        '<label>Faction<input value="' + escAttr(m.factionId) + '" disabled></label>' +
        '<label>Rank<input class="rankInput" type="number" value="' + Number(m.rank || 0) + '"></label>' +
        '<button class="secondary saveRank">Save</button><button class="danger removeFaction">Remove</button></div>'
      ).join('');
    }

    function factionAddForm() {
      return '<div class="fields">' + input('newFactionId', 'Faction ID', '') + input('newFactionRank', 'Rank', 0, 'number') +
        '</div><div class="actions"><button id="addFaction">Add or update faction</button></div>';
    }

    function bountyForm() {
      return '<div class="fields">' + input('bountyHoldId', 'Hold ID', '') + input('bountyAmount', 'Amount', 0, 'number') +
        '</div><div class="actions"><button id="setBounty">Set bounty</button></div>';
    }

    function actorPropertyForm() {
      const keys = (state.meta.knownActorProperties || []).map((k) => '<option value="' + escAttr(k) + '">' + esc(k) + '</option>').join('');
      return '<div class="fields"><label>Known key<select id="knownProp"><option value="">Custom</option>' + keys +
        '</select></label>' + input('propKey', 'Property key', '') + '</div><label style="margin-top:10px">JSON value<textarea id="propValue">null</textarea></label>' +
        '<div class="actions"><button id="setProperty">Set property</button></div>';
    }

    function factionDocForm() {
      return '<div class="fields">' + input('docFactionId', 'Faction ID', '') + '</div>' +
        '<label style="margin-top:10px">Benefits<textarea id="docBenefits"></textarea></label>' +
        '<label style="margin-top:10px">Burdens<textarea id="docBurdens"></textarea></label>' +
        '<label style="margin-top:10px">Bylaws<textarea id="docBylaws"></textarea></label>' +
        '<div class="actions"><button id="saveDoc">Save document</button></div>';
    }

    function bindForms() {
      el('saveAttrs').onclick = () => mutate('patchPlayer', {
        playerId: state.selectedId,
        patch: {
          name: val('name'), holdId: val('holdId'), hungerLevel: num('hungerLevel'), drunkLevel: num('drunkLevel'),
          septims: num('septims'), stipendPaidHours: num('stipendPaidHours'), minutesOnline: num('minutesOnline'),
          isStaff: checked('isStaff'), isLeader: checked('isLeader'), isDown: checked('isDown'), isCaptive: checked('isCaptive')
        }
      });
      el('addFaction').onclick = () => mutate('setFaction', { playerId: state.selectedId, factionId: val('newFactionId'), rank: num('newFactionRank') });
      el('setBounty').onclick = () => mutate('setBounty', { playerId: state.selectedId, holdId: val('bountyHoldId'), amount: num('bountyAmount') });
      el('knownProp').onchange = () => { if (el('knownProp').value) el('propKey').value = el('knownProp').value; };
      el('setProperty').onclick = () => mutate('setActorProperty', { playerId: state.selectedId, key: val('propKey'), value: parseJsonOrString(val('propValue')) });
      el('saveDoc').onclick = () => mutate('setFactionDocument', {
        factionId: val('docFactionId'), benefits: val('docBenefits'), burdens: val('docBurdens'), bylaws: val('docBylaws')
      }, false);
      document.querySelectorAll('.saveRank').forEach((btn) => btn.onclick = () => {
        const row = btn.closest('.membership');
        mutate('setFaction', { playerId: state.selectedId, factionId: row.dataset.faction, rank: Number(row.querySelector('.rankInput').value) });
      });
      document.querySelectorAll('.removeFaction').forEach((btn) => btn.onclick = () => {
        const row = btn.closest('.membership');
        mutate('removeFaction', { playerId: state.selectedId, factionId: row.dataset.faction });
      });
    }

    async function mutate(method, payload, refreshAfter) {
      try {
        await request('/admin/api/' + method, { method: 'POST', body: JSON.stringify(payload) });
        showNotice('Saved.', 'ok');
        if (refreshAfter !== false) await refresh(true);
      } catch (err) {
        showNotice(err.message, 'error');
      }
    }

    function input(id, label, value, type) {
      return '<label>' + label + '<input id="' + id + '" type="' + (type || 'text') + '" value="' + escAttr(value === null ? '' : value) + '"></label>';
    }
    function check(id, label, value) { return '<label><input id="' + id + '" type="checkbox" ' + (value ? 'checked' : '') + '>' + label + '</label>'; }
    function kv(k, v) { return '<div><span>' + esc(k) + '</span><strong>' + esc(String(v)) + '</strong></div>'; }
    function val(id) { return el(id).value.trim(); }
    function num(id) { return Number(el(id).value); }
    function checked(id) { return el(id).checked; }
    function parseJsonOrString(text) { try { return JSON.parse(text); } catch (_) { return text; } }
    function esc(value) { return String(value).replace(/[&<>"']/g, (c) => ({ '&': '&amp;', '<': '&lt;', '>': '&gt;', '"': '&quot;', "'": '&#39;' }[c])); }
    function escAttr(value) { return esc(value); }

    el('refreshBtn').onclick = () => refresh(false);
    el('autoBtn').onclick = () => {
      state.auto = !state.auto;
      el('autoBtn').textContent = 'Auto: ' + (state.auto ? 'on' : 'off');
      schedule();
    };
    function schedule() {
      if (state.timer) clearInterval(state.timer);
      state.timer = state.auto ? setInterval(() => refresh(true), 5000) : null;
    }
    refresh(true);
    schedule();
  </script>
</body>
</html>`;

const createApp = (getOriginPort: () => number) => {
  const app = new Koa();
  app.use(koaBody.default({ multipart: true }));

  app.use(async (ctx: any, next: any) => {
    try {
      await next();
    } catch (err: any) {
      if (401 === err.status) {
        ctx.status = 401;
        ctx.set("WWW-Authenticate", "Basic realm=\"skymp-server\"");
      } else {
        throw err;
      }
    }
  });

  const router = new Router();
  router.get(new RegExp("/scripts/.*"), (ctx: any) => ctx.throw(403));
  router.get(new RegExp("\.es[mpl]"), (ctx: any) => ctx.throw(403));
  router.get(new RegExp("\.bsa"), (ctx: any) => ctx.throw(403));

  router.use("/admin", adminGuard);
  router.get("/admin", (ctx: any) => {
    ctx.set("Content-Type", "text/html; charset=utf-8");
    ctx.body = adminDashboardHtml;
  });
  router.get("/admin/", (ctx: any) => {
    ctx.set("Content-Type", "text/html; charset=utf-8");
    ctx.body = adminDashboardHtml;
  });
  router.get("/admin/api/snapshot", (ctx: any) => {
    const { status, body } = runAdminApi("getSnapshot");
    ctx.status = status;
    ctx.body = body;
  });
  router.post("/admin/api/:method", (ctx: any) => {
    const { method } = ctx.params;
    const allowed = new Set([
      "patchPlayer",
      "setFaction",
      "removeFaction",
      "setBounty",
      "setActorProperty",
      "setFactionDocument",
    ]);
    if (!allowed.has(method)) {
      ctx.status = 404;
      ctx.body = { ok: false, error: `Unknown admin method: ${method}` };
      return;
    }
    const { status, body } = runAdminApi(method, ctx.request.body);
    ctx.status = status;
    ctx.body = body;
  });

  router.post("/rpc/:rpcClassName", (ctx: any) => {
    const { rpcClassName } = ctx.params;
    const { payload } = ctx.request.body;

    rpcCallsCounter.inc({ rpcClassName });
    const endTimer = rpcDurationHistogram.startTimer({ rpcClassName });

    try {
      if (gScampServer.onHttpRpcRunAttempt) {
        ctx.body = gScampServer.onHttpRpcRunAttempt(rpcClassName, payload);
      }
    } finally {
      endTimer();
    }
  });

  router.use('/metrics', (ctx: any, next: any) => {
    console.log(`Metrics requested by ${ctx.request.ip}`);
    return next();
  });

  if (metricsAuth) {
    if (metricsAuth.password !== DISABLE_AUTH_PASSWORD) {
      router.use("/metrics", auth({ name: metricsAuth.user, pass: metricsAuth.password }));
    }
    router.get("/metrics", async (ctx: any) => {
      ctx.set("Content-Type", register.contentType);
      ctx.body = await getAggregatedMetrics(gScampServer);
    });
  } else {
    router.get("/metrics", async (ctx: any) => {
      ctx.throw(401);
      console.error("Metrics endpoint is protected by authentication, but no credentials are configured");
    });
  }

  app.use(router.routes()).use(router.allowedMethods());
  app.use(serve("data"));
  return app;
};

export const setServer = (scampServer: any) => {
  gScampServer = scampServer;
};

export const noteUserConnected = (userId: number): void => {
  connectedUsers.set(userId, { connectedAt: Date.now() });
};

export const noteUserDisconnected = (userId: number): void => {
  connectedUsers.delete(userId);
};

export const main = (settings: Settings): void => {
  metricsAuthParse(settings);
  adminAuthParse(settings);
  const devServerPort = 1234;

  const uiListenHost = settings.allSettings.uiListenHost as (string | undefined);
  const uiPort = settings.port === 7777 ? 3000 : settings.port + 1;

  Axios({
    method: "get",
    url: `http://localhost:${devServerPort}`,
  })
    .then(() => {
      console.log(`UI dev server has been detected on port ${devServerPort}`);

      const state = { port: 0 };

      const appStatic = createApp(() => state.port);
      const srv = http.createServer(appStatic.callback());
      srv.listen(0, () => {
        const { port } = srv.address() as AddressInfo;
        state.port = port;
        const appProxy = new Koa();
        appProxy.use(
          proxy({
            host: `http://localhost:${devServerPort}`,
            map: (path: string) => {
              const resultPath = path.match(/^\/ui\/.*/)
                ? `http://localhost:${devServerPort}` + path.substr(3)
                : `http://localhost:${port}` + path;
              console.log(`proxy ${path} => ${resultPath}`);
              return resultPath;
            },
          })
        );
        console.log(`Server resources folder is listening on ${uiPort}`);
        http.createServer(appProxy.callback()).listen(uiPort, uiListenHost);
      });
    })
    .catch(() => {
      const app = createApp(() => uiPort);
      console.log(`Server resources folder is listening on ${uiPort}`);
      const server = http.createServer(app.callback());
      server.listen(uiPort, uiListenHost);
    });
};
