export { };
import * as sp from "skyrimPlatform";

//#region Timers declaration and implementation

interface Timer {
  handler: TimerHandler;
  args: any[];
  delayMs: number;
  passedMs: number;
}

let timersArr = new Array<Timer | null>();
let intervalsArr = new Array<Timer | null>();

globalThis.setTimeout = (handler: TimerHandler, timeout?: number, ...args: any[]): number => {
  const timer: Timer = { handler, args, delayMs: timeout ?? 0, passedMs: 0 };
  for (let i = 0; i < timersArr.length; ++i) {
    if (!timersArr[i]) {
      timersArr[i] = timer;
      return i + 1;
    }
  }

  return timersArr.push(timer);
}

globalThis.clearTimeout = (id: number | undefined): void => {
  if (id === undefined) {
    timersArr = new Array<Timer | null>();
    return;
  }

  if (id <= 0 || id > timersArr.length) return;
  timersArr[id - 1] = null;
  return;
}

globalThis.setInterval = (handler: TimerHandler, timeout?: number, ...args: any[]): number => {
  const timer: Timer = { handler, args, delayMs: timeout ?? 0, passedMs: 0 };
  for (let i = 0; i < intervalsArr.length; ++i) {
    if (!intervalsArr[i]) {
      intervalsArr[i] = timer;
      return i + 1;
    }
  }

  return intervalsArr.push(timer);
}

globalThis.clearInterval = (id: number | undefined): void => {
  if (id === undefined) {
    intervalsArr = new Array<Timer | null>();
    return;
  }

  if (id <= 0 || id > intervalsArr.length) return;
  intervalsArr[id - 1] = null;
  return;
}

//#endregion

//#region Timers processing

let updateEventHandle: sp.EventHandle | null;
let lastCallTime: number = Date.now();

const processTimers = (): void => {
  const dt = Date.now() - lastCallTime;
  lastCallTime = Date.now();

  // process timers
  for (let i = 0; i < timersArr.length; ++i) {
    const timer = timersArr[i];

    if (timer) {
      timer.passedMs += dt;
      if (timer.passedMs >= timer.delayMs) {
        timersArr[i] = null;
        if (typeof timer.handler === "function") {
          timer.handler.call(this, timer.args);
        } else {
          eval(timer.handler);
        }
      }
    } else if (i === timersArr.length - 1) {
      // remove last unused element
      timersArr.length -= 1;
    }
  }

  // process intervals
  for (let i = 0; i < intervalsArr.length; ++i) {
    const interval = intervalsArr[i];
    if (interval) {
      interval.passedMs += dt;
      if (interval.passedMs >= interval.delayMs) {
        interval.passedMs = 0;
        if (typeof interval.handler === "function") {
          interval.handler.call(this, interval.args);
        } else {
          eval(interval.handler);
        }
      }
    } else if (i === intervalsArr.length - 1) {
      // remove last unused element
      intervalsArr.length -= 1;
    }
  }
}

sp.on("menuOpen", (e) => {
  if (e.name === sp.Menu.Main) {
    if (updateEventHandle) {
      sp.unsubscribe(updateEventHandle);
    }

    updateEventHandle = sp.on("tick", () => processTimers());
  }
});

sp.on("preLoadGame", () => {
  if (updateEventHandle) {
    sp.unsubscribe(updateEventHandle);
  }

  updateEventHandle = sp.on("update", () => processTimers());
});

try {
  if (sp.Game.getPlayer()!) { };
  updateEventHandle = sp.on("update", () => processTimers());
}
catch {
  updateEventHandle = sp.on("tick", () => processTimers());
}

//#endregion
