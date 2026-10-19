import { logError } from '../../logging';
import {
  initializeClientVoiceSync,
  shutdownClientVoiceSync,
} from '../../sync/voiceActivitySync';
import { ConnectionMessage } from '../events/connectionMessage';
import { CustomPacketMessage } from '../messages/customPacketMessage';
import { ClientListener, CombinedController, Sp } from './clientListener';
import { parseSyncVoiceActivityPacket } from './voiceActivitySyncPacket';

export class VoiceActivitySyncService extends ClientListener {
  constructor(private sp: Sp, private controller: CombinedController) {
    super();

    initializeClientVoiceSync((handler) => {
      this.controller.emitter.on('customPacketMessage', (event) => {
        this.onCustomPacketMessage(event, handler);
      });
    });

    this.controller.emitter.on('connectionDisconnect', () => {
      shutdownClientVoiceSync();
    });
  }

  private onCustomPacketMessage(
    event: ConnectionMessage<CustomPacketMessage>,
    handler: (packet: unknown) => void,
  ): void {
    try {
      const packet = parseSyncVoiceActivityPacket(event.message.contentJsonDump);
      if (!packet) {
        return;
      }

      handler(packet);
    } catch (e) {
      if (e instanceof SyntaxError) {
        logError(this, 'Failed to parse customPacket JSON', e.message);
        return;
      }
      throw e;
    }
  }
}
