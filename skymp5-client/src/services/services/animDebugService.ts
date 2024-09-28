import { logTrace, logError } from "../../logging";
import { AnimDebugSettings } from "../messages_settings/animDebugSettings";
import { ClientListener, CombinedController, Sp } from "./clientListener";
import { ButtonEvent, CameraStateChangedEvent, DxScanCode, Menu } from "skyrimPlatform";

const playerId = 0x14;

// TODO: split into two separate services: AnimDebugService and <you name it: a service for 3rd person camera enforcement in anims>
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

    const self = this;
    this.sp.hooks.sendAnimationEvent.add({
      enter: (ctx) => { },
      leave: (ctx) => {
        self.onSendAnimationEventLeave(ctx);
      }
    }, playerId, playerId);

    if (!this.settings || !this.settings.isActive) return;

    if (this.settings.textOutput?.isActive) {
      this.queue = new AnimQueueCollection(this.sp, this.settings);
      this.sp.storage[AnimQueueCollection.name] = this.queue;
    }

    if (this.settings.animKeys) {
      logTrace(this, `Found animKeys in settings. Registering buttonEvent listener`);
      this.controller.on("buttonEvent", (e) => this.onButtonEvent(e));
    }
    else {
      logError(this, `No animKeys defined in settings`);
    }
  }

  private onSendAnimationEventLeave(ctx: { animEventName: string, animationSucceeded: boolean }) {
    const animLowerCase = ctx.animEventName.toLowerCase();
    if (animLowerCase.startsWith("idle") && !animLowerCase.startsWith("idleforcedefaultstate")) {
      this.controller.once("update", () => {
        // This is only for player.playidle
        if (!this.sp.Ui.isMenuOpen(Menu.Console)) return;

        if (this.sp.Game.getPlayer()?.getFurnitureReference()) return;

        logTrace(this, `Forcing third person and disabling player controls`);
        this.sp.Game.forceThirdPerson();
        this.sp.Game.disablePlayerControls(true, false, true, false, false, false, false, false, 0);
        this.needsExitingAnim = true;
        this.startAntiExploitPolling();
      });
    }

    if (this.queue === undefined) return;

    this.queue.push(ctx.animEventName, ctx.animationSucceeded ? animationSucceededTextColor : animationNotSucceededTextColor);
  }

  private onButtonEvent(e: ButtonEvent) {
    // TODO: de-hardcode controls
    if (e.code === DxScanCode.Spacebar
      || e.code === DxScanCode.W
      || e.code === DxScanCode.A
      || e.code === DxScanCode.S
      || e.code === DxScanCode.D) {

      if (this.needsExitingAnim) {
        this.exitAnim();
      }
    }
    else {
      if (this.needsExitingAnim) {
        this.sp.Debug.notification("Пробел, чтобы выйти из анимации");
      }
    }

    if (!e.isUp) return;

    if (!this.settings) return;

    if (!this.settings.animKeys![e.code]) return;

    if (this.sp.Game.getPlayer()?.isWeaponDrawn()) return;

    if (this.sp.Ui.isMenuOpen(Menu.Favorites)) return;
    if (this.sp.Ui.isMenuOpen(Menu.Console)) return;

    if (this.stopAnimInProgress) return;

    if (this.sp.Game.getPlayer()?.getFurnitureReference()) return;
    if (this.sp.Game.getPlayer()?.isSneaking()) return;
    if (this.sp.Game.getPlayer()?.isSwimming()) return;

    if (this.settings.animKeys![e.code].toLowerCase() === "idleforcedefaultstate") {
      if (this.needsExitingAnim) {
        this.exitAnim();
      }
    }
    else {
      this.sp.Game.forceThirdPerson();
      this.sp.Game.disablePlayerControls(true, false, true, false, false, false, false, false, 0);
      this.sp.Debug.sendAnimationEvent(this.sp.Game.getPlayer(), this.settings.animKeys![e.code]);

      this.needsExitingAnim = true;
      this.startAntiExploitPolling();
    }

    logTrace(this, `Sent animation event: ${this.settings.animKeys![e.code]}`);
  }

  private exitAnim() {
    this.sp.Debug.sendAnimationEvent(this.sp.Game.getPlayer(), "IdleForceDefaultState");
    logTrace(this, `Sent animation event: IdleForceDefaultState`);
    this.needsExitingAnim = false;

    this.stopAnimInProgress = true;
    this.sp.Utility.wait(0.5).then(() => {
      this.sp.Game.enablePlayerControls(true, false, true, false, false, false, false, false, 0);
    });
    this.sp.Utility.wait(1).then(() => {
      this.stopAnimInProgress = false;
    });
  }

  private startAntiExploitPolling() {
    // Fixes https://github.com/skyrim-multiplayer/skymp5-gamemode/issues/240
    // P.S. There is a very similar code in skymp5-gamemode
    // See disableCheats.ts, skymp5-gamemode for comments

    let _callNative = this.sp.callNative;
    let cameraState = -1;
    let needsExitingAnim = false;

    let f = (i: number): void => {
      if (i >= 10 * 60) {
        return;
      }

      cameraState = _callNative("Game", "getCameraState", undefined) as number;

      needsExitingAnim = this.needsExitingAnim;

      if (!needsExitingAnim) {
        return f(Infinity);
      }

      if (cameraState === 0) { // 1-st person
        _callNative("Game", "forceThirdPerson", undefined);
        this.exitAnim();
        return f(Infinity);
      }

      this.controller.once("update", () => f(i + 1));
    }

    f(0);
  }

  private queue?: AnimQueueCollection;
  private settings?: AnimDebugSettings;

  private needsExitingAnim = false;

  private stopAnimInProgress = false;
}

type AnimListItem = {
  name: string,
  textId: number,
  color: number[]
}

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
