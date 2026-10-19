import { Log, System, SystemContext } from './system';

export class WeatherSystem implements System {
  systemName = 'WeatherSystem';

  constructor(private log: Log) {}

  connect(userId: number, ctx: SystemContext): void {
    this.connectedUsers.add(userId);
    if (this.currentWeatherFormId === null) {
      return;
    }
    ctx.svr.sendCustomPacket(
      userId,
      JSON.stringify({
        customPacketType: 'syncWeather',
        formId: this.currentWeatherFormId,
      }),
    );
  }

  disconnect(userId: number, _ctx: SystemContext): void {
    this.connectedUsers.delete(userId);
  }

  customPacket(
    userId: number,
    type: string,
    content: Record<string, unknown>,
    ctx: SystemContext,
  ): void {
    if (type === 'setWeather') {
      const formId = content['formId'];
      if (typeof formId !== 'number') {
        return;
      }
      this.currentWeatherFormId = formId;
      this.log(`[WeatherSystem] Weather set to formId 0x${formId.toString(16)}`);
      this.broadcastWeather(ctx);
    } else if (type === 'clearWeather') {
      this.currentWeatherFormId = null;
      this.log('[WeatherSystem] Weather cleared (natural)');
      this.broadcastClear(ctx);
    }
  }

  private broadcastWeather(ctx: SystemContext): void {
    if (this.currentWeatherFormId === null) {
      return;
    }
    const packet = JSON.stringify({
      customPacketType: 'syncWeather',
      formId: this.currentWeatherFormId,
    });
    for (const uid of this.connectedUsers) {
      ctx.svr.sendCustomPacket(uid, packet);
    }
  }

  private broadcastClear(ctx: SystemContext): void {
    const packet = JSON.stringify({ customPacketType: 'clearWeather' });
    for (const uid of this.connectedUsers) {
      ctx.svr.sendCustomPacket(uid, packet);
    }
  }

  private currentWeatherFormId: number | null = null;
  private connectedUsers = new Set<number>();
}
