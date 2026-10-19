import { Log, System, SystemContext } from './system';

export interface TimeState {
  gameHour: number;
  gameDay: number;
  gameMonth: number;
  gameYear: number;
  timeScale: number;
  serverTimestamp: number;
}

export class TimeSystem implements System {
  systemName = 'TimeSystem';

  constructor(private log: Log, private timeScale = 20) {}

  initAsync(ctx: SystemContext): Promise<void> {
    this.state = this.buildStateFromRealTime();
    this.log(`[TimeSystem] Initialized with hour=${this.state.gameHour.toFixed(2)}, timeScale=${this.state.timeScale}`);
    return Promise.resolve();
  }

  updateAsync(ctx: SystemContext): Promise<void> {
    // Re-sync state from real time periodically so it stays accurate
    this.state = this.buildStateFromRealTime();
    return Promise.resolve();
  }

  connect(userId: number, ctx: SystemContext): void {
    this.sendState(userId, ctx);
  }

  private sendState(userId: number, ctx: SystemContext): void {
    const state = this.buildStateFromRealTime();
    ctx.svr.sendCustomPacket(
      userId,
      JSON.stringify({
        customPacketType: 'syncTime',
        gameHour: state.gameHour,
        gameDay: state.gameDay,
        gameMonth: state.gameMonth,
        gameYear: state.gameYear,
        timeScale: state.timeScale,
        serverTimestamp: state.serverTimestamp,
      }),
    );
  }

  private buildStateFromRealTime(): TimeState {
    const d = new Date(Date.now());
    const gameHour =
      d.getUTCHours() +
      d.getUTCMinutes() / 60 +
      d.getUTCSeconds() / 3600 +
      d.getUTCMilliseconds() / 3600000;

    return {
      gameHour,
      gameDay: d.getUTCDate(),
      gameMonth: d.getUTCMonth(),
      gameYear: d.getUTCFullYear() - 2020 + 199,
      timeScale: this.timeScale,
      serverTimestamp: Date.now(),
    };
  }

  private state: TimeState = {
    gameHour: 12,
    gameDay: 1,
    gameMonth: 0,
    gameYear: 199,
    timeScale: 1,
    serverTimestamp: Date.now(),
  };
}
