import { MsgType } from "../../messages";
import { logTrace, logError } from "../../logging";
import { ConnectionMessage } from "../events/connectionMessage";
import { CustomPacketMessage } from "../messages/customPacketMessage";
import { CustomPacketMessage2 } from "../messages/customPacketMessage2";
import { AnimDebugSettings } from "../messages_settings/animDebugSettings";
import { ClientListener, CombinedController, Sp } from "./clientListener";
import { ButtonEvent, CameraStateChangedEvent, DxScanCode, Menu, Message } from "skyrimPlatform";

const playerId = 0x14;

// ex AnimDebugService part
export class SweetCameraEnforcementService extends ClientListener {
    constructor(private sp: Sp, private controller: CombinedController) {
        super();

        const hasSweetPie = this.hasSweetPie();

        if (!hasSweetPie) {
            logTrace(this, "SweetTaffy features disabled");
        }
        else {
            logTrace(this, "SweetTaffy features enabled");
        }

        this.settings = this.sp.settings["skymp5-client"]["animDebug"] as AnimDebugSettings | undefined;

        if (hasSweetPie) {
            const self = this;
            this.sp.hooks.sendAnimationEvent.add({
                enter: (ctx) => { },
                leave: (ctx) => {
                    self.onSendAnimationEventLeave(ctx);
                }
            }, playerId, playerId);

            this.controller.on("buttonEvent", (e) => this.onButtonEvent(e));
            this.controller.emitter.on("customPacketMessage2", (e) => this.onCustomPacketMessage2(e));
        }
    }

    private onCustomPacketMessage2(e: ConnectionMessage<CustomPacketMessage2>) {
        if (e.message.content["customPacketType"] !== "invokeAnim") return;

        logTrace(this, "Received custom packet with customPacketType invokeAnim");

        const name = e.message.content["animEventName"];
        const requestId = e.message.content["requestId"];

        if (typeof name !== "string") {
            logError(this, "Expected animEventName to be string");
            return;
        }

        if (typeof requestId !== "string" && typeof requestId !== "number" && typeof requestId !== "undefined") {
            logError(this, "Expected requestId to be string or number or undefined");
            return;
        }

        const result = this.tryInvokeAnim(name, true);

        const message: CustomPacketMessage = {
            t: MsgType.CustomPacket,
            content: {
                customPacketType: "invokeAnimResult",
                result, requestId
            }
        };

        this.controller.emitter.emit("sendMessage", {
            message,
            reliability: "reliable"
        });
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

    private startAntiExploitPolling(mode: "no_death" | "death" = "death") {
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
                if (mode === "death") {
                    this.sp.Game.getPlayer()?.damageActorValue("Health", 10000);
                }
                return f(Infinity);
            }

            this.controller.once("update", () => f(i + 1));
        }

        f(0);
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

        if (!this.settings || !this.settings.animKeys) return;

        const animEvent = this.settings.animKeys[e.code];

        if (!animEvent) return;

        logTrace(this, "Starting anims from keyboard is disabled in this version")
        // this.tryInvokeAnim(animEvent, false);
    }

    private tryInvokeAnim(animEvent: string, weaponDrawnAllowed: boolean) {
        const player = this.sp.Game.getPlayer();

        if (!player) return { success: false, reason: "player_not_found" };

        if (player.isWeaponDrawn() && !weaponDrawnAllowed) return { success: false, reason: "weapon_drawn" };

        if (this.sp.Ui.isMenuOpen(Menu.Favorites)) return { success: false, reason: "favorites_menu_open" };
        if (this.sp.Ui.isMenuOpen(Menu.Console)) return { success: false, reason: "console_menu_open" };

        if (this.stopAnimInProgress) return { success: false, reason: "busy_stopping_anim" };

        if (player.getFurnitureReference()) return { success: false, reason: "player_in_furniture" };
        if (player.isSneaking()) return { success: false, reason: "player_sneaking" };
        if (player.isSwimming()) return { success: false, reason: "player_swimming" };

        if (animEvent.toLowerCase() === "idleforcedefaultstate") {
            if (this.needsExitingAnim) {
                this.exitAnim();
            }
        }
        else {
            this.sp.Game.forceThirdPerson();
            this.sp.Game.disablePlayerControls(true, false, true, false, false, false, false, false, 0);
            this.sp.Debug.sendAnimationEvent(this.sp.Game.getPlayer(), animEvent);

            this.needsExitingAnim = true;
            this.startAntiExploitPolling("no_death");
        }

        logTrace(this, `Sent animation event: ${animEvent}`);
        return { success: true };
    }

    private hasSweetPie(): boolean {
        const modCount = this.sp.Game.getModCount();
        for (let i = 0; i < modCount; ++i) {
            if (this.sp.Game.getModName(i).toLowerCase().includes('sweetpie')) {
                return true;
            }
        }
        return false;
    }

    private needsExitingAnim = false;
    private stopAnimInProgress = false;
    private settings?: AnimDebugSettings;
}
