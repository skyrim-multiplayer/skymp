import { Content, Log, System, SystemContext } from './system';

export class MovementDebugLogSystem implements System {
  systemName = 'MovementDebugLogSystem';

  constructor(private log: Log) {}

  customPacket(
    userId: number,
    type: string,
    content: Content,
    ctx: SystemContext,
  ): void {
    if (type !== 'movementDebugTelemetry') {
      return;
    }

    const now = Date.now();
    const last = this.lastLogAtByUser.get(userId) ?? 0;
    if (now - last < this.minLogIntervalMs) {
      return;
    }
    this.lastLogAtByUser.set(userId, now);

    const actorId = ctx.svr.getUserActor(userId);
    const payload = {
      type: 'movementDebugTelemetry',
      userId,
      actorId: actorId ? actorId.toString(16) : null,
      ip: this.getUserIpSafe(userId, ctx),
      ts: this.safeNumber(content['ts'], now),
      receivedAt: now,
      averagePacketIntervalMs: this.safeNumber(
        content['averagePacketIntervalMs'],
        0,
      ),
      extrapolationMs: this.safeNumber(content['extrapolationMs'], 0),
      hardCorrectionCount: this.safeNumber(content['hardCorrectionCount'], 0),
      lastSnapDistance: this.safeNumber(content['lastSnapDistance'], 0),
    };

    this.log(`[MovementDebug] ${JSON.stringify(payload)}`);
  }

  disconnect(userId: number): void {
    this.lastLogAtByUser.delete(userId);
  }

  private safeNumber(value: unknown, fallback: number): number {
    const num = Number(value);
    return Number.isFinite(num) ? num : fallback;
  }

  private getUserIpSafe(userId: number, ctx: SystemContext): string {
    try {
      return ctx.svr.getUserIp(userId);
    } catch {
      return '';
    }
  }

  private readonly minLogIntervalMs = 10000;
  private readonly lastLogAtByUser = new Map<number, number>();
}
