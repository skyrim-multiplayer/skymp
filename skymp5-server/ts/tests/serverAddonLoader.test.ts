import assert from "assert";
import { EventEmitter } from "events";
import * as path from "path";
import { loadServerAddonSystems } from "../addons/serverAddonLoader";
import { SystemContext } from "../systems/system";
import {
  getServerAddonLoaderTestEvents,
  resetServerAddonLoaderTestState,
} from "./fixtures/serverAddonLoaderTestState";

type MockServerOptions = {
  actorAngleZ?: number;
  actorAngleZThrows?: boolean;
};

const repoRoot = path.resolve(__dirname, "../../../../../../../");
const generatedServerRoot = path.resolve(repoRoot, "build/dist/server");

const fixturePath = (filename: string): string => {
  return path.resolve(repoRoot, `build/dist/server/dist_back/skymp5-server/ts/tests/fixtures/${filename}`);
};

const createLogCollector = (): {
  entries: unknown[][];
  log: (...args: unknown[]) => void;
} => {
  const entries: unknown[][] = [];
  return {
    entries,
    log: (...args: unknown[]) => {
      entries.push(args);
    },
  };
};

const expectFatalLoadError = async (
  promise: Promise<unknown>,
): Promise<void> => {
  await assert.rejects(promise, /startup aborted/i);
};

const expectNoSpawnListenersRemain = (ctx: SystemContext): void => {
  ctx.gm.emit("spawnAllowed", 7, 11);

  const residualEvents = getServerAddonLoaderTestEvents().filter((event) => {
    return event.kind === "spawnAllowed" || event.kind === "throwingSpawnAllowed";
  });
  assert.equal(residualEvents.length, 0);
};

const createContext = (options: MockServerOptions = {}): SystemContext => {
  const server = {
    sentCustomPackets: new Array<{
      jsonContent: string;
      userId: number;
    }>(),
    getActorAngleZ: (_actorId: number): number => {
      if (options.actorAngleZThrows) {
        throw new Error("actor angle unavailable");
      }

      return options.actorAngleZ ?? 0;
    },
    getActorCellOrWorld: (): number => 0,
    getActorPos: (): number[] => [0, 0, 0],
    getUserActor: (): number => 1,
    isConnected: (): boolean => true,
    sendCustomPacket: (userId: number, jsonContent: string): void => {
      server.sentCustomPackets.push({
        jsonContent,
        userId,
      });
    },
  };

  return {
    gm: new EventEmitter(),
    svr: server as unknown as SystemContext["svr"],
  };
};

const testDuplicateAddonIdsAreRejected = async () => {
  resetServerAddonLoaderTestState();
  const ctx = createContext({
    actorAngleZ: 90,
  });
  const logs = createLogCollector();

  await expectFatalLoadError(loadServerAddonSystems({
    ctx,
    log: logs.log,
    moduleBasePath: generatedServerRoot,
    settings: {
      addonModules: {
        server: [
          fixturePath("subscriptionsAddon.js"),
          fixturePath("duplicateAddon.js"),
        ],
      },
      addons: {
        "loader-fixture": {
          greeting: "hello",
        },
      },
    },
  }));

  assert.equal(
    getServerAddonLoaderTestEvents().filter((event) => event.kind === "duplicateCreateServerAddon").length,
    0,
  );
  assert.ok(getServerAddonLoaderTestEvents().some((event) => event.kind === "dispose"));
  assert.ok(logs.entries.some((entry) => String(entry[0]).includes("failed to load")));
  assert.ok(logs.entries.some((entry) => String(entry[1]).includes("Duplicate server addonId")));
  expectNoSpawnListenersRemain(ctx);
};

const testSyncEventHooksAndAsyncLifecycleHooks = async () => {
  resetServerAddonLoaderTestState();
  const ctx = createContext({
    actorAngleZ: 180,
  });
  const server = ctx.svr as unknown as {
    sentCustomPackets: Array<{
      jsonContent: string;
      userId: number;
    }>;
  };

  const systems = await loadServerAddonSystems({
    ctx,
    log: () => undefined,
    moduleBasePath: generatedServerRoot,
    settings: {
      addonModules: {
        server: [
          fixturePath("subscriptionsAddon.js"),
        ],
      },
      addons: {
        "loader-fixture": {
          greeting: "hello",
        },
      },
    },
  });

  assert.equal(systems.length, 1);
  await systems[0].initAsync?.(ctx);
  assert.equal(server.sentCustomPackets.length, 1);
  assert.equal(server.sentCustomPackets[0].userId, 7);
  assert.deepEqual(JSON.parse(server.sentCustomPackets[0].jsonContent), {
    customPacketType: "loader:init",
    greeting: "hello",
  });
  await systems[0].updateAsync?.(ctx);
  systems[0].connect?.(7, ctx);
  ctx.gm.emit("spawnAllowed", 7, 11);
  systems[0].customPacket?.(7, "loader:test", { ok: true }, ctx);
  let events = getServerAddonLoaderTestEvents();
  assert.equal(events.filter((event) => event.kind === "typedCustomPacket").length, 1);
  assert.equal(events.filter((event) => event.kind === "legacyCustomPacket").length, 1);

  systems[0].customPacket?.(7, "loader:other", { ok: false }, ctx);
  systems[0].disconnect?.(7, ctx);
  await systems[0].disposeAsync?.(ctx);

  events = getServerAddonLoaderTestEvents();
  assert.ok(events.some((event) => event.kind === "spawnAllowed"));
  assert.ok(events.some((event) => event.kind === "init"));
  assert.ok(events.some((event) => event.kind === "update"));
  assert.equal(events.filter((event) => event.kind === "connect").length, 1);
  assert.equal(events.filter((event) => event.kind === "typedCustomPacket").length, 1);
  assert.equal(events.filter((event) => event.kind === "legacyCustomPacket").length, 2);
  assert.equal(events.filter((event) => event.kind === "disconnect").length, 1);
  assert.ok(events.some((event) => event.kind === "dispose"));
};

const testActorAngleCapabilityFallsBackToNull = async () => {
  resetServerAddonLoaderTestState();
  const ctx = createContext({
    actorAngleZThrows: true,
  });

  const systems = await loadServerAddonSystems({
    ctx,
    log: () => undefined,
    moduleBasePath: generatedServerRoot,
    settings: {
      addonModules: {
        server: [
          fixturePath("subscriptionsAddon.js"),
        ],
      },
    },
  });

  await systems[0].initAsync?.(ctx);

  const initEvent = getServerAddonLoaderTestEvents().find((event) => event.kind === "init");
  assert.deepEqual(initEvent?.payload, {
    actorAngleSnapshot: null,
    greeting: "hi",
  });
};

const testMissingAddonModuleIsFatal = async () => {
  resetServerAddonLoaderTestState();
  const ctx = createContext();
  const logs = createLogCollector();

  await expectFatalLoadError(loadServerAddonSystems({
    ctx,
    log: logs.log,
    moduleBasePath: generatedServerRoot,
    settings: {
      addonModules: {
        server: [
          fixturePath("subscriptionsAddon.js"),
          fixturePath("missingAddon.js"),
        ],
      },
    },
  }));

  assert.ok(getServerAddonLoaderTestEvents().some((event) => event.kind === "dispose"));
  assert.ok(logs.entries.some((entry) => String(entry[1]).includes("Cannot find module")));
  expectNoSpawnListenersRemain(ctx);
};

const testBadAddonModuleShapeIsFatal = async () => {
  resetServerAddonLoaderTestState();
  const ctx = createContext();
  const logs = createLogCollector();

  await expectFatalLoadError(loadServerAddonSystems({
    ctx,
    log: logs.log,
    moduleBasePath: generatedServerRoot,
    settings: {
      addonModules: {
        server: [
          fixturePath("subscriptionsAddon.js"),
          fixturePath("badModule.js"),
        ],
      },
    },
  }));

  assert.ok(getServerAddonLoaderTestEvents().some((event) => event.kind === "dispose"));
  assert.ok(
    logs.entries.some((entry) => String(entry[1]).includes("Expected module to export createServerAddon")),
  );
  expectNoSpawnListenersRemain(ctx);
};

const testRelativeAddonPathResolvesFromSettingsDirectory = async () => {
  resetServerAddonLoaderTestState();
  const ctx = createContext({
    actorAngleZ: 45,
  });
  const originalCwd = process.cwd();
  const settingsDirectory = generatedServerRoot;
  const configuredModulePath = path.relative(settingsDirectory, fixturePath("subscriptionsAddon.js"));

  try {
    process.chdir(path.resolve(repoRoot, "skymp5-client"));

    const systems = await loadServerAddonSystems({
      ctx,
      log: () => undefined,
      moduleBasePath: settingsDirectory,
      settings: {
        addonModules: {
          server: [
            configuredModulePath,
          ],
        },
      },
    });

    assert.equal(systems.length, 1);
    await systems[0].initAsync?.(ctx);
    await systems[0].disposeAsync?.(ctx);
  } finally {
    process.chdir(originalCwd);
  }
};

const testCreateServerAddonExceptionIsFatalAndCleansUp = async () => {
  resetServerAddonLoaderTestState();
  const ctx = createContext();
  const logs = createLogCollector();

  await expectFatalLoadError(loadServerAddonSystems({
    ctx,
    log: logs.log,
    moduleBasePath: generatedServerRoot,
    settings: {
      addonModules: {
        server: [
          fixturePath("subscriptionsAddon.js"),
          fixturePath("throwingAddon.js"),
        ],
      },
    },
  }));

  const events = getServerAddonLoaderTestEvents();
  assert.ok(events.some((event) => event.kind === "dispose"));
  assert.equal(events.filter((event) => event.kind === "throwingCreateServerAddon").length, 1);
  assert.ok(logs.entries.some((entry) => String(entry[1]).includes("createServerAddon boom")));
  expectNoSpawnListenersRemain(ctx);
};

export const runServerAddonLoaderTests = async (): Promise<void> => {
  await testDuplicateAddonIdsAreRejected();
  await testSyncEventHooksAndAsyncLifecycleHooks();
  await testActorAngleCapabilityFallsBackToNull();
  await testMissingAddonModuleIsFatal();
  await testBadAddonModuleShapeIsFatal();
  await testRelativeAddonPathResolvesFromSettingsDirectory();
  await testCreateServerAddonExceptionIsFatalAndCleansUp();

  console.log("serverAddonLoader.test.ts passed");
};

if (require.main === module) {
  void runServerAddonLoaderTests().catch((error) => {
    console.error(error);
    process.exit(1);
  });
}
