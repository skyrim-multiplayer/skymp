/*
 * Barebones active gamemode.
 *
 * Keep this file small and easy to reload while experimenting.
 * See `gamemode.example.js` for a slightly richer example.
 */

const state = globalThis.__skympBarebonesGamemodeState || (
  globalThis.__skympBarebonesGamemodeState = {
    bootCount: 0,
    lastLoadedAt: null,
  }
);

function log(message) {
  console.log(`[gamemode] ${message}`);
}

function toHex(formId) {
  if (typeof formId !== "number") {
    return String(formId);
  }

  return `0x${formId.toString(16)}`;
}

function bootstrap() {
  state.bootCount += 1;
  state.lastLoadedAt = new Date().toISOString();

  mp.onDeath = (actorId, killerId) => {
    log(`onDeath actor=${toHex(actorId)} killer=${toHex(killerId)}`);
  };

  mp.onRespawn = (actorId) => {
    log(`onRespawn actor=${toHex(actorId)}`);
  };

  globalThis.describeGamemode = () => {
    log(JSON.stringify(state, null, 2));
  };

  log(`loaded (boot ${state.bootCount})`);
}

bootstrap();
