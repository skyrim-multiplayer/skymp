import * as sp from "skyrimPlatform";

export interface AnimDebugSettings {
  isActive?: boolean;
  animListCount?: number;
  animListStartPos?: { x: number, y: number },
  animListYPosDelta?: number;
  animKeys?: { [index: number]: string };
}

type AnimListItem = {
  name: string,
  textId: number,
}

const playerId = 0x14;

class AnimQueueCollection {
  public static readonly Name = "AnimQueueCollection";

  constructor(settings: AnimDebugSettings) {
    const animListCount = settings.animListCount ?? 5;
    const animListStartPos = settings.animListStartPos ?? { x: 400, y: 600 };
    const animListYPosDelta = settings.animListYPosDelta ?? 40;
    const textColor = [255, 255, 255, 1];
    let y = animListStartPos.y;

    this.list = new Array<AnimListItem>(animListCount);
    for (let idx = 0; idx < animListCount; ++idx) {
      this.list[idx] = { name: "", textId: sp.createText(animListStartPos.x, y, "", textColor) };
      y += animListYPosDelta;
    }
  }

  private readonly list: Array<AnimListItem>;

  public clearSPText(): void {
    if (this.list.length === 0) return;
    this.list.forEach(item => sp.destroyText(item.textId));
  }

  public push(animName: string): void {
    let prevAnimName: string | null = null;
    for (let idx = this.list.length - 1; idx >= 0; --idx) {
      const item = this.list[idx];
      prevAnimName = item.name;
      item.name = animName;
      sp.setTextString(item.textId, animName);
      animName = prevAnimName;
    }
  }
}

if (sp.storage[AnimQueueCollection.Name] && (sp.storage[AnimQueueCollection.Name] as AnimQueueCollection).clearSPText) {
  (sp.storage[AnimQueueCollection.Name] as AnimQueueCollection)?.clearSPText();
}

export const init = (settings: AnimDebugSettings): void => {
  if (!settings || !settings.isActive) return;

  var queue = new AnimQueueCollection(settings);
  sp.storage[AnimQueueCollection.name] = queue;

  sp.hooks.sendAnimationEvent.add({
    enter: (ctx) => { },
    leave: (ctx) => {
      if (ctx.animationSucceeded) {
        queue.push(ctx.animEventName);
      }
    }
  }, playerId, playerId);

  if (settings.animKeys) {
    sp.on("buttonEvent", (e) => {
      if (e.isUp && settings.animKeys![e.code]) {
        sp.Debug.sendAnimationEvent(sp.Game.getPlayer()!, settings.animKeys![e.code]);
      }
    });
  }
}
