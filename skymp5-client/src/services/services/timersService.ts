import { EventHandle, Menu, MenuOpenEvent } from "skyrimPlatform";
import { ClientListener, CombinedController, Sp } from "./clientListener";

interface Timer {
  handler: TimerHandler;
  args: any[];
  delayMs: number;
  passedMs: number;
}

enum ProcessMethodType {
  update = "update",
  tick = "tick"
}

export class TimersService extends ClientListener {
  constructor(private sp: Sp, private controller: CombinedController) {
    super();

    const storageProcessMethod = sp.storage[this.processMethodTypeStorageKey];
    if (typeof storageProcessMethod !== "function") {
      this.setProcessMethod(storageProcessMethod as ProcessMethodType);
    } else {
      this.setProcessMethod(ProcessMethodType.tick);
    }

    this.controller.on("menuOpen", (e) => this.onMenuOpen(e));
    this.controller.on("preLoadGame", () => this.onPreLoadGame());
  }

  setTimeout(handler: TimerHandler, timeout?: number, ...args: any[]): number {
    const timer: Timer = { handler, args, delayMs: timeout ?? 0, passedMs: 0 };
    for (let i = 0; i < this.timersArr.length; ++i) {
      if (!this.timersArr[i]) {
        this.timersArr[i] = timer;
        return i + 1;
      }
    }

    return this.timersArr.push(timer);
  }

  clearTimeout(id: number | undefined): void {
    if (id === undefined) {
      this.timersArr = new Array<Timer | null>();
      return;
    }

    if (id <= 0 || id > this.timersArr.length) return;
    this.timersArr[id - 1] = null;
    return;
  }

  setInterval(handler: TimerHandler, timeout?: number, ...args: any[]): number {
    const timer: Timer = { handler, args, delayMs: timeout ?? 0, passedMs: 0 };
    for (let i = 0; i < this.intervalsArr.length; ++i) {
      if (!this.intervalsArr[i]) {
        this.intervalsArr[i] = timer;
        return i + 1;
      }
    }

    return this.intervalsArr.push(timer);
  }

  clearInterval(id: number | undefined): void {
    if (id === undefined) {
      this.intervalsArr = new Array<Timer | null>();
      return;
    }

    if (id <= 0 || id > this.intervalsArr.length) return;
    this.intervalsArr[id - 1] = null;
    return;
  }

  private setProcessMethod(method: ProcessMethodType): void {
    switch (method) {
      case ProcessMethodType.tick:
        this.sp.storage[this.processMethodTypeStorageKey] = ProcessMethodType.tick;
        this.updateEventHandle = this.controller.on(ProcessMethodType.tick, () => this.processTimers());
        return;
      case ProcessMethodType.update:
        this.sp.storage[this.processMethodTypeStorageKey] = ProcessMethodType.update;
        this.updateEventHandle = this.controller.on(ProcessMethodType.update, () => this.processTimers());
        return;
      default:
        break;
    }

    try {
      if (this.sp.Game.getPlayer()!) { };
      this.setProcessMethod(ProcessMethodType.update);
    } catch {
      this.setProcessMethod(ProcessMethodType.tick);
    }
  }

  private processTimers(): void {
    const dt = Date.now() - this.lastCallTime;
    this.lastCallTime = Date.now();

    // process timers
    for (let i = 0; i < this.timersArr.length; ++i) {
      const timer = this.timersArr[i];

      if (timer) {
        timer.passedMs += dt;
        if (timer.passedMs >= timer.delayMs) {
          this.timersArr[i] = null;
          if (typeof timer.handler === "function") {
            timer.handler.call(this, timer.args);
          } else {
            eval(timer.handler);
          }
        }
      } else if (i === this.timersArr.length - 1) {
        // remove last unused element
        this.timersArr.length -= 1;
      }
    }

    // process intervals
    for (let i = 0; i < this.intervalsArr.length; ++i) {
      const interval = this.intervalsArr[i];
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
      } else if (i === this.intervalsArr.length - 1) {
        // remove last unused element
        this.intervalsArr.length -= 1;
      }
    }
  }

  private onMenuOpen(e: MenuOpenEvent) {
    if (e.name === Menu.Main) {
      if (this.updateEventHandle) {
        this.sp.unsubscribe(this.updateEventHandle);
      }

      this.setProcessMethod(ProcessMethodType.tick);
    }

  }

  private onPreLoadGame() {
    if (this.updateEventHandle) {
      this.sp.unsubscribe(this.updateEventHandle);
    }

    this.setProcessMethod(ProcessMethodType.update);
  }

  private timersArr = new Array<Timer | null>();
  private intervalsArr = new Array<Timer | null>();
  private updateEventHandle?: EventHandle;
  private lastCallTime: number = Date.now();

  private readonly processMethodTypeStorageKey = "updateTypeStorageKey";
}
