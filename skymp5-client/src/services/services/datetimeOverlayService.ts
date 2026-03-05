import { logTrace, logError } from "../../logging";
import { DatetimeOverlaySettings } from "../messages_settings/animDebugSettings";
import { ClientListener, CombinedController, Sp } from "./clientListener";

const playerId = 0x14;

export class DatetimeOverlayService extends ClientListener {
  constructor(private sp: Sp, private controller: CombinedController) {
    super();

    this.settings = this.sp.settings["skymp5-client"]["datetimeOverlay"] as DatetimeOverlaySettings | undefined;

    // clear previous texts in case of hotreload
    if (this.sp.storage[DatettimeOverlayState.Name] && (this.sp.storage[DatettimeOverlayState.Name] as DatettimeOverlayState).destroy) {
      logTrace(this, `Destroying old state`);
      try {
        (this.sp.storage[DatettimeOverlayState.Name] as DatettimeOverlayState).destroy();
      } catch (e) {
        logError(this, `Failed to destroy old state:`, e);
      }
    }

    if (!this.settings || !this.settings.isActive) {
      return;
    }

    this.state = new DatettimeOverlayState(this.sp);
    this.controller.on('update', () => this.state?.update());
  }

  private state?: DatettimeOverlayState;
  private settings?: DatetimeOverlaySettings;
}

class DatettimeOverlayState {
  public static readonly Name = "DatettimeOverlayState";
  private readonly textId: number;

  constructor(private sp: Sp) {
    this.textId = sp.createText(1920 / 2, 20, "", [255, 255, 255, 1]);
    sp.setTextSize(this.textId, 14);
  }

  public destroy(): void {
    this.sp.destroyText(this.textId);
  }

  public update(): void {
    this.sp.setTextString(this.textId, new Date().toLocaleString('ru-RU'));
  }
}
