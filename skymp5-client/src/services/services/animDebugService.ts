import { logTrace, logError } from "../../logging";
import { ClientListener, CombinedController, Sp } from "./clientListener";
import { ButtonEvent } from "skyrimPlatform";

export class AnimDebugService extends ClientListener {
  constructor(private sp: Sp, private controller: CombinedController) {
    super();

    this.settings = this.sp.settings["skymp5-client"]["animDebug"] as AnimDebugSettings | undefined;

    // clear previous texts in case of hotreload
    if (this.sp.storage[AnimQueueCollection.Name] && (this.sp.storage[AnimQueueCollection.Name] as AnimQueueCollection).clearSPText) {
      logTrace(this, `Destroying old AnimQueueCollection`);
      try {
        (this.sp.storage[AnimQueueCollection.Name] as AnimQueueCollection).clearSPText();
      }
      catch (e) {
        logError(this, `Failed to destroy old AnimQueueCollection:`, e);
      }
    }

    if (!this.settings || !this.settings.isActive) return;

    if (this.settings.textOutput?.isActive) {
      this.queue = new AnimQueueCollection(this.sp, this.settings);
      this.sp.storage[AnimQueueCollection.name] = this.queue;

      const self = this;
      this.sp.hooks.sendAnimationEvent.add({
        enter: (ctx) => { },
        leave: (ctx) => {
          self.onSendAnimationEventLeave(ctx);
        }
      }, playerId, playerId);
    }

    if (this.settings.animKeys) {
      this.controller.on("buttonEvent", (e) => this.onButtonEvent(e));
    }
  }

  private onSendAnimationEventLeave(ctx: { animEventName: string, animationSucceeded: boolean }) {
    if (this.queue === undefined) return;

    this.queue.push(ctx.animEventName, ctx.animationSucceeded ? animationSucceededTextColor : animationNotSucceededTextColor);
  }

  private onButtonEvent(e: ButtonEvent) {
    if (e.isUp && this.settings && this.settings.animKeys![e.code]) {
      this.sp.Debug.sendAnimationEvent(this.sp.Game.getPlayer(), this.settings.animKeys![e.code]);
    }
  }

  private queue?: AnimQueueCollection;
  private settings?: AnimDebugSettings;
}

interface AnimTextOutput {
  isActive?: boolean;
  itemCount?: number;
  startPos?: { x: number, y: number },
  yPosDelta?: number,
}

interface AnimDebugSettings {
  isActive?: boolean;
  textOutput?: AnimTextOutput,
  animKeys?: { [index: number]: string };
}

type AnimListItem = {
  name: string,
  textId: number,
  color: number[]
}

const playerId = 0x14;
const animationSucceededTextColor = [255, 255, 255, 1];
const animationNotSucceededTextColor = [255, 0, 0, 1];

class AnimQueueCollection {
  public static readonly Name = "AnimQueueCollection";

  constructor(private sp: Sp, settings: AnimDebugSettings) {
    const arrayLength = settings?.textOutput?.itemCount ?? 5;
    const startPos = settings?.textOutput?.startPos ?? { x: 650, y: 600 };;
    const yPosDelta = settings?.textOutput?.yPosDelta ?? 32;

    let y = startPos.y;

    this.list = new Array<AnimListItem>(arrayLength);
    for (let idx = 0; idx < arrayLength; ++idx) {
      this.list[idx] = { name: "", textId: sp.createText(startPos.x, y, "", animationSucceededTextColor), color: animationSucceededTextColor };
      y += yPosDelta;
    }
  }

  private readonly list: Array<AnimListItem>;

  public clearSPText(): void {
    if (this.list.length === 0) return;
    this.list.forEach(item => this.sp.destroyText(item.textId));
  }

  public push(animName: string, color: number[]): void {
    let prevItem: AnimListItem | null = null;
    for (let idx = this.list.length - 1; idx >= 0; --idx) {
      const item = this.list[idx];
      prevItem = { ...item };

      item.name = animName;
      item.color = color;
      this.sp.setTextString(item.textId, item.name);
      this.sp.setTextColor(item.textId, item.color);

      animName = prevItem.name;
      color = prevItem.color;
    }
  }
}
