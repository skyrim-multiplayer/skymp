import assert from "assert";
import {
  ClientAddonAuthGameData,
  ClientAddonBrowserApi,
  ClientAddonCapabilities,
} from "../../skymp5-addons-api/clientAddonHost";
import { ClientAddonHostRuntime } from "../src/services/services/clientAddonHostRuntime";

type RuntimeHarness = {
  authCalls: ClientAddonAuthGameData[];
  browserCalls: Array<{
    kind: string;
    payload?: unknown;
  }>;
  errors: unknown[][];
  runtime: ClientAddonHostRuntime;
};

const createHarness = (
  capabilities: ClientAddonCapabilities,
): RuntimeHarness => {
  const authCalls: ClientAddonAuthGameData[] = [];
  const browserCalls: Array<{
    kind: string;
    payload?: unknown;
  }> = [];
  const errors: unknown[][] = [];
  const browser: ClientAddonBrowserApi = {
    emitEvent: (eventName: string, dataJson: string) => {
      browserCalls.push({
        kind: "emitEvent",
        payload: { dataJson, eventName },
      });
    },
    getBackendName: () => "test",
    isFocused: () => false,
    isVisible: () => false,
    loadUrl: (url: string) => {
      browserCalls.push({
        kind: "loadUrl",
        payload: url,
      });
    },
    setFocused: (focused: boolean) => {
      browserCalls.push({
        kind: "setFocused",
        payload: focused,
      });
    },
    setVisible: (visible: boolean) => {
      browserCalls.push({
        kind: "setVisible",
        payload: visible,
      });
    },
  };

  return {
    authCalls,
    browserCalls,
    errors,
    runtime: new ClientAddonHostRuntime({
      auth: {
        submitAuthAttempt: (authGameData) => {
          authCalls.push(authGameData);
        },
      },
      browser,
      capabilities,
      getLocalProfileId: () => 101,
      getSettingsScope: () => undefined,
      log: () => undefined,
      logError: (...args: unknown[]) => {
        errors.push(args);
      },
      resolveScanCode: () => 55,
      sendCustomPacket: () => undefined,
    }),
  };
};

const testDuplicateAddonIdRegistration = () => {
  const harness = createHarness({
  });
  let initCalls = 0;

  harness.runtime.registerAddon("example", () => {
    initCalls += 1;
  });
  harness.runtime.registerAddon(" example ", () => {
    initCalls += 1;
  });

  assert.equal(initCalls, 1);
  assert.equal(harness.errors.length, 1);
  assert.match(String(harness.errors[0][1]), /duplicate client addon registration/i);
};

const testLocalSpawnReplayIsErrorIsolated = () => {
  const harness = createHarness({
  });
  let spawnCalls = 0;

  harness.runtime.dispatchLocalSpawn({ profileId: 9001 });
  harness.runtime.registerAddon("spawn-safe", (api) => {
    api.onLocalSpawn(() => {
      spawnCalls += 1;
      throw new Error("boom");
    });
  });

  assert.equal(spawnCalls, 1);
  assert.equal(harness.errors.length, 1);
  assert.match(String(harness.errors[0][1]), /local spawn handler failed/i);
};

const testCapabilitiesAreExposedToAddons = () => {
  const harness = createHarness({});
  let seenCapabilities: ClientAddonCapabilities | null = null;

  harness.runtime.registerAddon("capabilities", (api) => {
    seenCapabilities = api.capabilities;
  });

  assert.deepEqual(seenCapabilities, {});
};

const testAuthBridgeIsExposedToAddons = () => {
  const harness = createHarness({});
  let authNeededCalls = 0;
  let connectionAcceptedCalls = 0;
  let connectionDisconnectCalls = 0;
  let connectionFailedCalls = 0;
  let deniedError = "";

  harness.runtime.registerAddon("auth", (api) => {
    api.auth.onAuthNeeded(() => {
      authNeededCalls += 1;
    });
    api.auth.onConnectionAccepted(() => {
      connectionAcceptedCalls += 1;
    });
    api.auth.onConnectionDenied((event) => {
      deniedError = event.error;
    });
    api.auth.onConnectionDisconnect(() => {
      connectionDisconnectCalls += 1;
    });
    api.auth.onConnectionFailed(() => {
      connectionFailedCalls += 1;
    });

    api.auth.submitAuthAttempt({
      local: {
        profileId: 777,
      },
    });
  });

  harness.runtime.dispatchAuthNeeded();
  harness.runtime.dispatchConnectionAccepted();
  harness.runtime.dispatchConnectionDenied({ error: "invalid password" });
  harness.runtime.dispatchConnectionDisconnect();
  harness.runtime.dispatchConnectionFailed();

  assert.deepEqual(harness.authCalls, [{
    local: {
      profileId: 777,
    },
  }]);
  assert.equal(authNeededCalls, 1);
  assert.equal(connectionAcceptedCalls, 1);
  assert.equal(connectionDisconnectCalls, 1);
  assert.equal(connectionFailedCalls, 1);
  assert.equal(deniedError, "invalid password");
};

testDuplicateAddonIdRegistration();
testLocalSpawnReplayIsErrorIsolated();
testCapabilitiesAreExposedToAddons();
testAuthBridgeIsExposedToAddons();

console.log("clientAddonHostRuntime.test.ts passed");
