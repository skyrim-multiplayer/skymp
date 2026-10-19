import Axios from 'axios';

import {
  DEFAULT_TIMEOUT_MS,
  MAX_REQUEST_BODY_BYTES,
  MAX_RESPONSE_BODY_BYTES,
} from '../lib/axiosDefaults';
import { ScampServer } from '../scampNative';
import { Log, System } from './system';
import { SystemContext } from './system';

interface MasterHeartbeatPayload {
  name: string;
  ip: string;
  port: number;
  maxPlayers: number;
  online: number;
  gamemode?: string;
  countryCode?: string;
  server_uid?: string;
}

export class MasterClient implements System {
  systemName = 'MasterClient';

  constructor(
    private log: Log,
    private serverPort: number,
    private serverIp: string,
    private masterUrl: string | null,
    private maxPlayers: number,
    private name: string,
    private masterKey: string,
    updateIntervalMs = 15000,
    private offlineMode = false,
    private gamemode?: string,
    private countryCode?: string,
    private serverUid?: string,
  ) {
    this.updateIntervalMs =
      MasterClient.clampHeartbeatInterval(updateIntervalMs);
  }

  private readonly updateIntervalMs: number;

  async initAsync(): Promise<void> {
    if (!this.masterUrl) {
      this.log('No master server specified');
      return;
    }

    this.log(`Using master server on ${this.masterUrl}`);

    this.endpoint = `${this.masterUrl}/api/servers/${this.masterKey}`;
    this.log(`Our endpoint on master is ${this.endpoint}`);

    await this.sendHeartbeat(0, 'initial');

    process.once('SIGINT', () => {
      void this.sendHeartbeat(0, 'shutdown');
    });
    process.once('SIGTERM', () => {
      void this.sendHeartbeat(0, 'shutdown');
    });
  }

  update(): void {
    return;
  }

  async updateAsync(ctx: SystemContext): Promise<void> {
    if (this.offlineMode) {
      return;
    }

    await new Promise((r) => setTimeout(r, this.updateIntervalMs));

    await this.sendHeartbeat(this.getCurrentOnline(ctx.svr), 'periodic');
  }

  // connect/disconnect events are not reliable so we do full recalculate
  private getCurrentOnline(svr: ScampServer): number {
    return (svr as any).get(0, 'onlinePlayers').length;
  }

  customPacket(): void {
    return;
  }

  private buildPayload(online: number): MasterHeartbeatPayload {
    const payload: MasterHeartbeatPayload = {
      name: this.name,
      ip: this.serverIp,
      port: this.serverPort,
      maxPlayers: this.maxPlayers,
      online,
    };

    if (this.gamemode) {
      payload.gamemode = this.gamemode;
    }
    if (this.countryCode) {
      payload.countryCode = this.countryCode;
    }
    if (this.serverUid) {
      payload.server_uid = this.serverUid;
    }

    return payload;
  }

  private async sendHeartbeat(
    online: number,
    reason: 'initial' | 'periodic' | 'shutdown',
  ): Promise<void> {
    if (!this.endpoint) {
      return;
    }

    const payload = this.buildPayload(online);
    const maxAttempts = 4;

    for (let attempt = 1; attempt <= maxAttempts; attempt++) {
      try {
        await Axios.post(this.endpoint, payload, {
          timeout: DEFAULT_TIMEOUT_MS,
          maxBodyLength: MAX_REQUEST_BODY_BYTES,
          maxContentLength: MAX_RESPONSE_BODY_BYTES,
        });
        return;
      } catch (error) {
        if (Axios.isAxiosError(error)) {
          if (error.response) {
            const body =
              typeof error.response.data === 'string'
                ? error.response.data
                : JSON.stringify(error.response.data);
            this.log(
              `[MasterClient] Heartbeat ${reason} failed with status=${error.response.status}, body=${body}`,
            );
            return;
          }

          const message = error.message || 'network error';
          this.log(
            `[MasterClient] Heartbeat ${reason} network error on attempt ${attempt}/${maxAttempts}: ${message}`,
          );
        } else {
          this.log(
            `[MasterClient] Heartbeat ${reason} failed on attempt ${attempt}/${maxAttempts}: ${String(
              error,
            )}`,
          );
        }

        if (attempt >= maxAttempts) {
          return;
        }

        const delayMs = MasterClient.getRetryDelayMs(attempt);
        await new Promise((resolve) => setTimeout(resolve, delayMs));
      }
    }
  }

  private static getRetryDelayMs(attempt: number): number {
    const baseMs = 500;
    const jitter = Math.floor(Math.random() * 250);
    return Math.min(5000, baseMs * 2 ** (attempt - 1)) + jitter;
  }

  private static clampHeartbeatInterval(value: number): number {
    if (!Number.isFinite(value)) {
      return 15000;
    }
    return Math.max(10000, Math.min(30000, Math.floor(value)));
  }

  private endpoint: string | null = null;
}
