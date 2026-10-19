import { logError } from '../../logging';
import { MsgType } from '../../messages';
import { CustomPacketMessage } from '../messages/customPacketMessage';
import { ClientListener, CombinedController, Sp } from './clientListener';
import { NetworkingService } from './networkingService';
import { FormView } from '../../view/formView';

export class MovementDebugTelemetryService extends ClientListener {
  constructor(private sp: Sp, private controller: CombinedController) {
    super();
    this.controller.on('update', () => this.onUpdate());
  }

  private onUpdate(): void {
    if (!this.isEnabled()) {
      return;
    }
    if (!this.isConnected()) {
      return;
    }

    const now = Date.now();
    if (now - this.lastSentAt < this.sendIntervalMs) {
      return;
    }
    this.lastSentAt = now;

    const stats = FormView.getMovementDebugStats();
    const message: CustomPacketMessage = {
      t: MsgType.CustomPacket,
      contentJsonDump: JSON.stringify({
        customPacketType: 'movementDebugTelemetry',
        ts: now,
        averagePacketIntervalMs: Math.round(stats.averagePacketIntervalMs),
        extrapolationMs: Math.round(stats.extrapolationMs),
        hardCorrectionCount: stats.hardCorrectionCount,
        lastSnapDistance: Math.round(stats.lastSnapDistance),
      }),
    };

    try {
      this.controller.emitter.emit('sendMessage', {
        message,
        reliability: 'unreliable',
      });
    } catch (e) {
      logError(this, 'failed to send movement debug telemetry', String(e));
    }
  }

  private isEnabled(): boolean {
    const cfg = this.sp.settings['skymp5-client'] as
      | Record<string, unknown>
      | undefined;
    const value = cfg?.['send-movement-debug-telemetry'];
    return typeof value === 'boolean' ? value : true;
  }

  private isConnected(): boolean {
    try {
      return this.controller.lookupListener(NetworkingService).isConnected();
    } catch {
      return false;
    }
  }

  private readonly sendIntervalMs = 10000;
  private lastSentAt = 0;
}
