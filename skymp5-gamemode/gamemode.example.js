/*
 * Richer example gamemode for local experimentation.
 *
 * Demonstrates:
 * - reload-safe state on globalThis
 * - server-side hooks (`mp.onDeath`, `mp.onRespawn`)
 * - a synced property via `mp.makeProperty`
 * - a client-generated event via `mp.makeEventSource`
 * - a few debug helpers exposed on globalThis
 *
 * This file is an example and is not the default load target.
 * The server normally loads `./skymp5-gamemode/gamemode.js` by default
 * when the `skymp5-gamemode` folder exists next to `server-settings.json`.
 */

const LOG_PREFIX = "[example-gamemode]";
const LOCAL_DEATH_COUNT_PROPERTY = "private.example.localDeathCount";
const EXAMPLE_INITIALIZED_PROPERTY = "private.example.initialized";
const LOCAL_DEATH_EVENT_NAME = "_onLocalDeathExample";

const state = globalThis.__skympExampleGamemodeState || (
  globalThis.__skympExampleGamemodeState = {
    bootCount: 0,
    lastLoadedAt: null,
    localDeathEventsHandled: 0,
  }
);

function log(message) {
  console.log(`${LOG_PREFIX} ${message}`);
}

function toHex(formId) {
  if (typeof formId !== "number") {
    return String(formId);
  }

  return `0x${formId.toString(16)}`;
}

function safeGet(formId, propertyName) {
  try {
    return mp.get(formId, propertyName);
  } catch (error) {
    log(`Failed to read ${propertyName} for ${toHex(formId)}: ${error}`);
    return undefined;
  }
}

function safeSet(formId, propertyName, value) {
  try {
    mp.set(formId, propertyName, value);
    return true;
  } catch (error) {
    log(`Failed to write ${propertyName} for ${toHex(formId)}: ${error}`);
    return false;
  }
}

function ensureTrackedPlayerState(formId) {
  if (safeGet(formId, EXAMPLE_INITIALIZED_PROPERTY)) {
    return;
  }

  safeSet(formId, EXAMPLE_INITIALIZED_PROPERTY, true);
  if (typeof safeGet(formId, LOCAL_DEATH_COUNT_PROPERTY) !== "number") {
    safeSet(formId, LOCAL_DEATH_COUNT_PROPERTY, 0);
  }
}

function installProperties() {
  mp.makeProperty(LOCAL_DEATH_COUNT_PROPERTY, {
    isVisibleByOwner: true,
    isVisibleByNeighbors: false,
    updateOwner: `
      const nextValue = typeof ctx.value === "number" ? ctx.value : 0;
      if (ctx.state.exampleDeathCountLastSeen !== nextValue) {
        ctx.state.exampleDeathCountLastSeen = nextValue;
        ctx.sp.printConsole("[example-gamemode] Local death count: " + nextValue);
      }
    `,
    updateNeighbor: "",
  });

  mp.makeProperty(EXAMPLE_INITIALIZED_PROPERTY, {
    isVisibleByOwner: false,
    isVisibleByNeighbors: false,
    updateOwner: "",
    updateNeighbor: "",
  });
}

function installEventSources() {
  // Convert the local player's death into a server event so we can keep
  // per-player state on the server side.
  mp.makeEventSource(LOCAL_DEATH_EVENT_NAME, `
    ctx.sp.on("update", () => {
      const player = ctx.sp.Game.getPlayer();
      if (!player) {
        return;
      }

      const isDead = player.getActorValuePercentage("health") === 0;
      if (ctx.state.exampleWasDead !== isDead) {
        if (isDead) {
          ctx.sendEvent();
        }
        ctx.state.exampleWasDead = isDead;
      }
    });
  `);

  mp[LOCAL_DEATH_EVENT_NAME] = (pcFormId) => {
    ensureTrackedPlayerState(pcFormId);

    const currentValue = safeGet(pcFormId, LOCAL_DEATH_COUNT_PROPERTY);
    const nextValue = typeof currentValue === "number" ? currentValue + 1 : 1;

    if (safeSet(pcFormId, LOCAL_DEATH_COUNT_PROPERTY, nextValue)) {
      state.localDeathEventsHandled += 1;
      log(`Handled ${LOCAL_DEATH_EVENT_NAME} for ${toHex(pcFormId)} -> ${nextValue}`);
    }
  };
}

function installServerHooks() {
  // This is intentionally tiny: just enough to show the shape of the hooks.
  mp.onDeath = (actorId, killerId) => {
    log(`mp.onDeath actor=${toHex(actorId)} killer=${toHex(killerId)}`);
  };

  mp.onRespawn = (actorId) => {
    log(`mp.onRespawn actor=${toHex(actorId)}`);
  };
}

function installDebugHelpers() {
  globalThis.describeExampleGamemode = () => {
    log(JSON.stringify({
      bootCount: state.bootCount,
      lastLoadedAt: state.lastLoadedAt,
      localDeathEventsHandled: state.localDeathEventsHandled,
    }, null, 2));
  };

  globalThis.describeExampleActor = (formId) => {
    log(JSON.stringify({
      formId: toHex(formId),
      type: safeGet(formId, "type"),
      pos: safeGet(formId, "pos"),
      localDeathCount: safeGet(formId, LOCAL_DEATH_COUNT_PROPERTY),
    }, null, 2));
  };

  globalThis.resetExamplePlayerState = (formId) => {
    ensureTrackedPlayerState(formId);
    safeSet(formId, LOCAL_DEATH_COUNT_PROPERTY, 0);
    log(`Reset ${LOCAL_DEATH_COUNT_PROPERTY} for ${toHex(formId)}`);
  };
}

function bootstrap() {
  installProperties();
  installEventSources();
  installServerHooks();
  installDebugHelpers();

  state.bootCount += 1;
  state.lastLoadedAt = new Date().toISOString();

  log(`Loaded example gamemode (boot ${state.bootCount})`);
  log(
    "Available helpers: describeExampleGamemode(), " +
    "describeExampleActor(formId), resetExamplePlayerState(formId)",
  );
}

bootstrap();
