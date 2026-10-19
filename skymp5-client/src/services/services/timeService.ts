import { ClientListener, CombinedController, Sp } from './clientListener';
import { ConnectionMessage } from '../events/connectionMessage';
import { CustomPacketMessage } from '../messages/customPacketMessage';
import { logError } from '../../logging';

interface TimeAnchor {
  gameHour: number;
  gameDay: number;
  gameMonth: number;
  gameYear: number;
  timeScale: number;
  receivedAt: number;
}

export class TimeService extends ClientListener {
  constructor(private sp: Sp, private controller: CombinedController) {
    super();
    controller.on('update', () => this.onUpdate());
    controller.emitter.on('customPacketMessage', (e) =>
      this.onCustomPacketMessage(e),
    );
  }

  public getTime() {
    if (this.serverAnchor) {
      const elapsedHours =
        ((Date.now() - this.serverAnchor.receivedAt) / 1000 / 3600) *
        this.serverAnchor.timeScale;
      const newGameHourValue =
        (this.serverAnchor.gameHour + elapsedHours) % 24;
      return {
        newGameHourValue,
        gameDay: this.serverAnchor.gameDay,
        gameMonth: this.serverAnchor.gameMonth,
        gameYear: this.serverAnchor.gameYear,
        timeScale: this.serverAnchor.timeScale,
      };
    }

    // Fallback: use real UTC time (legacy behaviour)
    const hoursOffsetSetting = this.sp.settings['skymp5-client']['hoursOffset'];
    const hoursOffset =
      typeof hoursOffsetSetting === 'number' ? hoursOffsetSetting : 0;
    const hoursOffsetMs = hoursOffset * 60 * 60 * 1000;
    const d = new Date(Date.now() + hoursOffsetMs);

    let newGameHourValue = 0;
    newGameHourValue += d.getUTCHours();
    newGameHourValue += d.getUTCMinutes() / 60;
    newGameHourValue += d.getUTCSeconds() / 60 / 60;
    newGameHourValue += d.getUTCMilliseconds() / 60 / 60 / 1000;
    return {
      newGameHourValue,
      gameDay: d.getUTCDate(),
      gameMonth: d.getUTCMonth(),
      gameYear: d.getUTCFullYear() - 2020 + 199,
      timeScale: 1,
    };
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

    if (content['customPacketType'] !== 'syncTime') {
      return;
    }

    const gameHour = content['gameHour'];
    const gameDay = content['gameDay'];
    const gameMonth = content['gameMonth'];
    const gameYear = content['gameYear'];
    const timeScale = content['timeScale'];

    if (
      typeof gameHour !== 'number' ||
      typeof gameDay !== 'number' ||
      typeof gameMonth !== 'number' ||
      typeof gameYear !== 'number' ||
      typeof timeScale !== 'number'
    ) {
      return;
    }

    this.serverAnchor = {
      gameHour,
      gameDay,
      gameMonth,
      gameYear,
      timeScale,
      receivedAt: Date.now(),
    };
  }

  private every2seconds() {
    const gameHourId = 0x38;
    const gameMonthId = 0x36;
    const gameDayId = 0x37;
    const gameYearId = 0x35;
    const timeScaleId = 0x3a;

    const gameHour = this.sp.GlobalVariable.from(
      this.sp.Game.getFormEx(gameHourId),
    );
    const gameDay = this.sp.GlobalVariable.from(
      this.sp.Game.getFormEx(gameDayId),
    );
    const gameMonth = this.sp.GlobalVariable.from(
      this.sp.Game.getFormEx(gameMonthId),
    );
    const gameYear = this.sp.GlobalVariable.from(
      this.sp.Game.getFormEx(gameYearId),
    );
    const timeScale = this.sp.GlobalVariable.from(
      this.sp.Game.getFormEx(timeScaleId),
    );

    if (!gameHour || !gameDay || !gameMonth || !gameYear || !timeScale) {
      return;
    }

    const t = this.getTime();

    const diff = Math.abs(gameHour.getValue() - t.newGameHourValue);
    if (diff >= 1 / 60) {
      gameHour.setValue(t.newGameHourValue);
      gameDay.setValue(t.gameDay);
      gameMonth.setValue(t.gameMonth);
      gameYear.setValue(t.gameYear);
    }

    timeScale.setValue(t.timeScale);
  }

  private onUpdate() {
    if (Date.now() - this.lastTimeUpd <= 2000) {
      return;
    }
    this.lastTimeUpd = Date.now();
    this.every2seconds();
  }

  private lastTimeUpd = 0;
  private serverAnchor: TimeAnchor | null = null;
}
