import { storage } from "skyrimPlatform";

storage["hostAttempts"] = [];

export const tryHost = (targetRemoteId: number): void => {
  storage["hostAttempts"].push(targetRemoteId);
};

export const nextHostAttempt = (): number | undefined => {
  const arr = storage["hostAttempts"] as Array<number>;
  if (arr.length === 0) return undefined;
  return arr.shift();
};
