type ServerAddonLoaderTestState = {
  events: Array<{
    kind: string;
    payload?: unknown;
  }>;
};

declare global {
  // eslint-disable-next-line no-var
  var __skympServerAddonLoaderTestState: ServerAddonLoaderTestState | undefined;
}

const ensureState = (): ServerAddonLoaderTestState => {
  if (!globalThis.__skympServerAddonLoaderTestState) {
    globalThis.__skympServerAddonLoaderTestState = {
      events: [],
    };
  }

  return globalThis.__skympServerAddonLoaderTestState;
};

export const pushServerAddonLoaderTestEvent = (
  kind: string,
  payload?: unknown,
): void => {
  ensureState().events.push({
    kind,
    payload,
  });
};

export const getServerAddonLoaderTestEvents = (): Array<{
  kind: string;
  payload?: unknown;
}> => {
  return [...ensureState().events];
};

export const resetServerAddonLoaderTestState = (): void => {
  ensureState().events = [];
};
