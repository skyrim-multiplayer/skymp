import { logError, logTrace } from '../../logging';
import { MsgType } from '../../messages';
import { ConnectionMessage } from '../events/connectionMessage';
import { CustomPacketMessage } from '../messages/customPacketMessage';
import { ClientListener, CombinedController, Sp } from './clientListener';
import { NetworkingService } from './networkingService';

interface RuntimeEventPayload {
  name: string;
  level: 'info' | 'warn' | 'error';
  ts: number;
  details?: string;
}

export class ClientRuntimeTelemetryService extends ClientListener {
  private readonly sessionId = `${Date.now().toString(36)}-${Math.random()
    .toString(36)
    .slice(2, 10)}`;
  private heartbeatTimer: ReturnType<typeof setInterval> | null = null;
  private lastSentAt = 0;

  constructor(private sp: Sp, private controller: CombinedController) {
    super();

    this.controller.emitter.on('connectionAccepted', () => {
      this.sendRuntimeEvent(
        'connectionAccepted',
        'info',
        this.describeTargetPeer(),
      );
      this.startHeartbeat();
    });

    this.controller.emitter.on('connectionDenied', (event) => {
      const reason = typeof event.error === 'string' ? event.error : 'unknown';
      this.sendRuntimeEvent('connectionDenied', 'warn', reason);
      this.stopHeartbeat();
    });

    this.controller.emitter.on('connectionFailed', () => {
      this.sendRuntimeEvent(
        'connectionFailed',
        'error',
        this.describeTargetPeer(),
      );
      this.stopHeartbeat();
    });

    this.controller.emitter.on('connectionDisconnect', () => {
      this.sendRuntimeEvent(
        'connectionDisconnect',
        'warn',
        this.describeTargetPeer(),
      );
      this.stopHeartbeat();
    });

    this.controller.emitter.on('customPacketMessage', (event) =>
      this.onCustomPacket(event),
    );
  }

  private startHeartbeat(): void {
    if (this.heartbeatTimer) return;

    this.heartbeatTimer = setInterval(() => {
      if (!this.isConnected()) return;
      this.sendRuntimeEvent('heartbeat', 'info', this.describeTargetPeer());
    }, 30000);
  }

  private stopHeartbeat(): void {
    if (!this.heartbeatTimer) return;
    clearInterval(this.heartbeatTimer);
    this.heartbeatTimer = null;
  }

  private isConnected(): boolean {
    try {
      return this.controller.lookupListener(NetworkingService).isConnected();
    } catch {
      return false;
    }
  }

  private describeTargetPeer(): string {
    const host = String(this.sp.storage?.targetIp || '');
    const port = Number(this.sp.storage?.targetPort || 0);
    if (host && Number.isFinite(port) && port > 0) {
      return `${host}:${port}`;
    }
    return 'target-peer-unknown';
  }

  private onCustomPacket(event: ConnectionMessage<CustomPacketMessage>): void {
    let content: Record<string, unknown> = {};

    try {
      content = JSON.parse(event.message.contentJsonDump);
    } catch (err) {
      if (err instanceof SyntaxError) {
        this.sendRuntimeEvent(
          'incomingCustomPacketInvalidJson',
          'warn',
          String(err.message || 'json parse failed'),
        );
        return;
      }
      throw err;
    }

    const packetType = String(content.customPacketType || '');
    if (!packetType.startsWith('loginFailed')) return;

    const reason =
      typeof content.reason === 'string' ? content.reason : packetType;

    this.sendRuntimeEvent(packetType, 'warn', reason);
  }

  private sendRuntimeEvent(
    name: string,
    level: 'info' | 'warn' | 'error',
    details?: string,
  ): void {
    if (!this.isConnected()) return;

    // Keep packet volume bounded when repetitive events fire in a tight loop.
    const now = Date.now();
    if (now - this.lastSentAt < 200) return;
    this.lastSentAt = now;

    const payload: RuntimeEventPayload = {
      name: String(name || 'unknown').slice(0, 120),
      level,
      ts: now,
      details: typeof details === 'string' ? details.slice(0, 300) : undefined,
    };

    const message: CustomPacketMessage = {
      t: MsgType.CustomPacket,
      contentJsonDump: JSON.stringify({
        customPacketType: 'clientTelemetry',
        source: 'skymp5-client',
        sessionId: this.sessionId,
        events: [payload],
      }),
    };

    try {
      this.controller.emitter.emit('sendMessage', {
        message,
        reliability: 'reliable',
      });
      logTrace(this, `sent runtime telemetry event ${payload.name}`);
    } catch (error) {
      logError(
        this,
        `failed to send runtime telemetry event ${payload.name}`,
        String(error),
      );
    }
  }
}
