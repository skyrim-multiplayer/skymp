import { NeverError } from './errors';
import * as sp from "skyrimPlatform";

export type eventType =
  "update" |
  "tick";

export type action = () => void;

export const on = (eventName: eventType, callback: action): number => {
  subscriptionMap.set(++subscriptionSequence, null);
  once(eventName, callback, subscriptionSequence);
  return subscriptionSequence;
}

export const off = (subNum: number): boolean => {
  return subscriptionMap.delete(subNum);
}

const subscriptionMap = new Map<number, null>();
let subscriptionSequence = 0;

const once = (eventName: eventType, callback: action, subNum: number): void => {
  if (!subscriptionMap.has(subNum)) return;
  switch (eventName) {
    case "tick":
      sp.once("tick", () => {
        callback();
        once(eventName, callback, subNum);
      });
      break;
    case "update":
      sp.once("update", () => {
        callback();
        once(eventName, callback, subNum);
      });
      break;
    default:
      throw new NeverError(eventName);
  }
}
