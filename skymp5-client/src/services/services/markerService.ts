import { MsgType } from '../../messages';
import { logError, logTrace } from '../../logging';
import { ConnectionMessage } from '../events/connectionMessage';
import { CustomPacketMessage } from '../messages/customPacketMessage';
import { ClientListener, CombinedController, Sp } from './clientListener';
import { showMapMarker } from 'skyrimPlatform';

export class MarkerService extends ClientListener {
  constructor(private sp: Sp, private controller: CombinedController) {
    super();

    this.controller.on('locationDiscovery', (e) => {
      if (!e.markerFormId) {
        return;
      }

      logTrace(this, 'Location discovered:', e.name, 'formId:', e.markerFormId.toString(16));

      const message: CustomPacketMessage = {
        t: MsgType.CustomPacket,
        contentJsonDump: JSON.stringify({
          customPacketType: 'markerDiscovered',
          markerFormId: e.markerFormId,
        }),
      };

      this.controller.emitter.emit('sendMessage', {
        message,
        reliability: 'reliable',
      });
    });

    this.controller.emitter.on('customPacketMessage', (e) =>
      this.onCustomPacketMessage(e),
    );
  }

  private onCustomPacketMessage(
    event: ConnectionMessage<CustomPacketMessage>,
  ): void {
    let content: Record<string, unknown> = {};
    try {
      content = JSON.parse(event.message.contentJsonDump);
    } catch (e) {
      if (e instanceof SyntaxError) {
        logError(this, 'Failed to parse customPacket JSON', e.message);
        return;
      }
      throw e;
    }

    if (content['customPacketType'] !== 'restoreMarkers') {
      return;
    }

    const formIds = content['markerFormIds'];
    if (!Array.isArray(formIds)) {
      return;
    }

    logTrace(this, 'Restoring', formIds.length, 'discovered markers');

    for (const formId of formIds) {
      if (typeof formId !== 'number') {
        continue;
      }
      try {
        showMapMarker(formId, true);
      } catch (e) {
        logError(this, 'Failed to restore marker formId:', formId.toString(16), e);
      }
    }
  }
}
