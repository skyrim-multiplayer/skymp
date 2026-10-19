import { ClientListener, CombinedController, Sp } from './clientListener';
import { ConnectionMessage } from '../events/connectionMessage';
import { CustomPacketMessage } from '../messages/customPacketMessage';
import { logError, logTrace } from '../../logging';

export class WeatherService extends ClientListener {
  constructor(private sp: Sp, private controller: CombinedController) {
    super();
    controller.emitter.on('customPacketMessage', (e) =>
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

    const type = content['customPacketType'];

    if (type === 'syncWeather') {
      const formId = content['formId'];
      if (typeof formId !== 'number') {
        return;
      }
      logTrace(this, 'Applying server weather formId:', formId.toString(16));
      const form = this.sp.Game.getFormEx(formId);
      const weather = this.sp.Weather.from(form);
      if (!weather) {
        logError(this, 'syncWeather: no Weather form found for formId', formId.toString(16));
        return;
      }
      weather.setActive(true, true);
    } else if (type === 'clearWeather') {
      logTrace(this, 'Clearing server weather override');
      this.sp.Weather.releaseOverride();
    }
  }
}
