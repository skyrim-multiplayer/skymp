import { MsgType } from "../../messages";
import { logTrace, logError } from "../../logging";
import { ConnectionMessage } from "../events/connectionMessage";
import { CustomPacketMessage } from "../messages/customPacketMessage";
import { CustomPacketMessage2 } from "../messages/customPacketMessage2";
import { AnimDebugSettings } from "../messages_settings/animDebugSettings";
import { ClientListener, CombinedController, Sp } from "./clientListener";
import { ButtonEvent, CameraStateChangedEvent, DxScanCode, Menu, Message } from "skyrimPlatform";

const playerId = 0x14;

interface InvokeAnimOptions {
    weaponDrawnAllowed: unknown;
    furnitureAllowed: unknown;
    exitAnimName: unknown;
    interruptAnimName: unknown;
    timeMs: unknown;
    isPlayExitAnimAfterwardsEnabled: unknown;
    parentAnimEventName: unknown;
    enablePlayerControlsDelayMs: unknown;
    preferInterruptAnimAsExitAnimTimeMs: unknown;
}

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

        let weaponDrawnAllowed = e.message.content["weaponDrawnAllowed"];
        let furnitureAllowed = e.message.content["furnitureAllowed"];
        let exitAnimName = e.message.content["exitAnimName"];
        let interruptAnimName = e.message.content["interruptAnimName"];
        let timeMs = e.message.content["timeMs"];
        let isPlayExitAnimAfterwardsEnabled = e.message.content["isPlayExitAnimAfterwardsEnabled"];
        let parentAnimEventName = e.message.content["parentAnimEventName"];
        let enablePlayerControlsDelayMs = e.message.content["enablePlayerControlsDelayMs"];
        let preferInterruptAnimAsExitAnimTimeMs = e.message.content["preferInterruptAnimAsExitAnimTimeMs"];

        if (typeof name !== "string") {
            logError(this, "Expected animEventName to be string");
            return;
        }

        if (typeof requestId !== "string" && typeof requestId !== "number" && typeof requestId !== "undefined") {
            logError(this, "Expected requestId to be string or number or undefined");
            return;
        }

        this.controller.once("update", () => {
            logTrace(this,
                "Trying to invoke anim", name,
                "with weaponDrawnAllowed=", weaponDrawnAllowed,
                ", furnitureAllowed=", furnitureAllowed,
                ", exitAnimName=", exitAnimName,
                ", interruptAnimName=", interruptAnimName,
                ", timeMs=", timeMs,
                ", isPlayExitAnimAfterwardsEnabled=", isPlayExitAnimAfterwardsEnabled,
                ", parentAnimEventName=", parentAnimEventName,
                ", enablePlayerControlsDelayMs=", enablePlayerControlsDelayMs,
                ", preferInterruptAnimAsExitAnimTimeMs=", preferInterruptAnimAsExitAnimTimeMs
            );

            const result = this.tryInvokeAnim(name, { weaponDrawnAllowed, furnitureAllowed, exitAnimName, interruptAnimName, timeMs, isPlayExitAnimAfterwardsEnabled, parentAnimEventName, enablePlayerControlsDelayMs, preferInterruptAnimAsExitAnimTimeMs });

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
                this.currentAnim = { name: ctx.animEventName, options: null, preferInterruptAnimState: null };
                this.startAntiExploitPolling();
            });
        }
    }

    private exitAnim(options: { playExitAnim: boolean, useInterruptAnimAsExitAnim?: boolean, cb?: () => void, enablePlayerControlsDelayMs: unknown }) {
        // TODO(1): See another TODO(1) in this file
        let usePlayerControlsDelayMs = true;

        if (options.playExitAnim) {
            let useInterruptAnimAsExitAnim = false;

            if (options.useInterruptAnimAsExitAnim) {
                useInterruptAnimAsExitAnim = true;
                usePlayerControlsDelayMs = false;
                logTrace(this, `Using interrupt anim as exit anim: ${this.interruptAnimName}. Reason: useInterruptAnimAsExitAnim is true`);
            }

            if (this.currentAnim?.preferInterruptAnimState?.prefer) {
                useInterruptAnimAsExitAnim = true;
                usePlayerControlsDelayMs = false;
                logTrace(this, `Using interrupt anim as exit anim: ${this.interruptAnimName}. Reason: preferInterruptAnimState.prefer is true`);
            }

            const animEventToSend = useInterruptAnimAsExitAnim ? (this.interruptAnimName || "IdleStop") : (this.exitAnimName || "IdleForceDefaultState");
            this.sp.Debug.sendAnimationEvent(this.sp.Game.getPlayer(), animEventToSend);
            logTrace(this, `Sent animation event:`, animEventToSend);
        }

        this.currentAnim = null;

        const enablePlayerControlsDelaySeconds = (typeof options.enablePlayerControlsDelayMs === "number"
            && options.enablePlayerControlsDelayMs > 0
            && Number.isFinite(options.enablePlayerControlsDelayMs)
            && usePlayerControlsDelayMs)
            ? options.enablePlayerControlsDelayMs / 1000
            : 0.5;
        const stopAnimDelaySeconds = enablePlayerControlsDelaySeconds + 0.5;

        this.stopAnimInProgress = true;
        this.sp.Utility.wait(enablePlayerControlsDelaySeconds).then(() => {
            this.sp.Game.enablePlayerControls(true, false, true, false, false, false, false, false, 0);
        });
        this.sp.Utility.wait(stopAnimDelaySeconds).then(() => {
            this.stopAnimInProgress = false;
            if (options.cb) {
                options.cb();
            }
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
                this.exitAnim({ playExitAnim: true, enablePlayerControlsDelayMs: undefined });
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
                if (this.currentAnim?.options) {
                    this.exitAnim({ playExitAnim: true, enablePlayerControlsDelayMs: this.currentAnim.options.enablePlayerControlsDelayMs });
                }
                else {
                    this.exitAnim({ playExitAnim: true, enablePlayerControlsDelayMs: undefined });
                }
            }
        }
        else {
            if (this.needsExitingAnim) {
                const intervalMs = this.settings?.exitAnimNotificationIntervalMs;
                if (!intervalMs || (Date.now() - this.lastNotificationMoment) >= intervalMs) {
                    this.lastNotificationMoment = Date.now();
                    this.sp.Debug.notification("Пробел, чтобы выйти из анимации");
                }
            }
        }

        if (!e.isUp) return;

        if (!this.settings || !this.settings.animKeys) return;

        const animEvent = this.settings.animKeys[e.code];

        if (!animEvent) return;

        logTrace(this, "Starting anims from keyboard is disabled in this version")
        // this.tryInvokeAnim(animEvent, {
        //     weaponDrawnAllowed: false,
        //     furnitureAllowed: false,
        //     exitAnimName: null,
        //     interruptAnimName: null,
        //     timeMs: 0,
        //     isPlayExitAnimAfterwardsEnabled: true,
        //     parentAnimEventName: null,
        //     enablePlayerControlsDelayMs: null,
        //     preferInterruptAnimAsExitAnimTimeMs: null
        // });
    }

    private tryInvokeAnim(animEvent: string, options: InvokeAnimOptions): { success: boolean, reason?: string } {
        this.tryInvokeAnimCount++;
        if (this.tryInvokeAnimCount >= this.tryInvokeAnimCountMax) {
            this.tryInvokeAnimCount = 0;
        }

        const currentInvokeAnimCount = this.tryInvokeAnimCount;

        if (typeof options.exitAnimName === "string") {
            this.exitAnimName = options.exitAnimName;
        }
        else {
            this.exitAnimName = null;
        }

        if (typeof options.interruptAnimName === "string") {
            this.interruptAnimName = options.interruptAnimName;
        }
        else {
            this.interruptAnimName = null;
        }

        if (typeof options.timeMs === "number" && options.timeMs > 0 && Number.isFinite(options.timeMs)) {
            const timeSeconds = options.timeMs / 1000;

            // Using Utility.wait makes sense because it waits game time, not simply real time.
            this.sp.Utility.wait(timeSeconds).then(() => {
                if (this.tryInvokeAnimCount !== currentInvokeAnimCount) {
                    return;
                }

                if (!this.needsExitingAnim) {
                    return;
                }

                const isPlayExitAnimAfterwardsEnabled = typeof options.isPlayExitAnimAfterwardsEnabled === "boolean" ? options.isPlayExitAnimAfterwardsEnabled : true;

                this.exitAnim({ playExitAnim: isPlayExitAnimAfterwardsEnabled, enablePlayerControlsDelayMs: options.enablePlayerControlsDelayMs });
            });
        }

        const player = this.sp.Game.getPlayer();

        if (!player) return { success: false, reason: "player_not_found" };

        if (player.isWeaponDrawn() && !options.weaponDrawnAllowed) return { success: false, reason: "weapon_drawn" };

        if (this.sp.Ui.isMenuOpen(Menu.Favorites)) return { success: false, reason: "favorites_menu_open" };
        if (this.sp.Ui.isMenuOpen(Menu.Console)) return { success: false, reason: "console_menu_open" };

        if (this.stopAnimInProgress) return { success: false, reason: "busy_stopping_anim" };

        if (player.getFurnitureReference() && !options.furnitureAllowed) return { success: false, reason: "player_in_furniture" };
        if (player.isSneaking()) return { success: false, reason: "player_sneaking" };
        if (player.isSwimming()) return { success: false, reason: "player_swimming" };

        if (animEvent.toLowerCase() === "idleforcedefaultstate") {
            if (this.needsExitingAnim) {
                this.exitAnim({ playExitAnim: true, enablePlayerControlsDelayMs: undefined });
            }
        }
        else {
            const f = () => {
                this.sp.Game.forceThirdPerson();
                this.sp.Game.disablePlayerControls(true, false, true, false, false, false, false, false, 0);
                this.sp.Debug.sendAnimationEvent(this.sp.Game.getPlayer(), animEvent);

                let preferInterruptAnimState: { prefer: boolean } | null = null;

                if (typeof options.preferInterruptAnimAsExitAnimTimeMs === "number" && options.preferInterruptAnimAsExitAnimTimeMs > 0 && Number.isFinite(options.preferInterruptAnimAsExitAnimTimeMs)) {
                    const state = { prefer: true };
                    logTrace(this, `Prefer interrupt anim as exit anim for ${options.preferInterruptAnimAsExitAnimTimeMs} ms`);
                    this.sp.Utility.wait(options.preferInterruptAnimAsExitAnimTimeMs / 1000).then(() => {
                        state.prefer = false;
                        logTrace(this, `Prefer interrupt anim as exit anim state changed to false`);
                    });
                    preferInterruptAnimState = state;
                }

                this.currentAnim = { name: animEvent, options, preferInterruptAnimState };
                this.startAntiExploitPolling("no_death");
            }

            if (typeof options.parentAnimEventName === "string" && this.currentAnimName === options.parentAnimEventName) {
                f();
            } else if (Array.isArray(options.parentAnimEventName) && options.parentAnimEventName.includes(this.currentAnimName)) {
                f();
            } else if (this.needsExitingAnim) {
                // TODO (1): consider not using enablePlayerControlsDelayMs here, because useInterruptAnimAsExitAnim doesn't need it
                this.exitAnim({ playExitAnim: true, useInterruptAnimAsExitAnim: true, cb: f, enablePlayerControlsDelayMs: options.enablePlayerControlsDelayMs });
            } else {
                f();
            }

            logTrace(this, `Sent animation event: ${animEvent} `);
        }

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

    get needsExitingAnim(): boolean {
        return this.currentAnimName !== null;
    }

    get currentAnimName(): string | null {
        return this.currentAnim ? this.currentAnim.name : null;
    }

    private currentAnim: {
        name: string,
        options: InvokeAnimOptions | null,
        preferInterruptAnimState: { prefer: boolean } | null
    } | null = null;

    private stopAnimInProgress = false;
    private settings?: AnimDebugSettings;
    private exitAnimName: string | null = null;
    private interruptAnimName: string | null = null;
    private tryInvokeAnimCount: number = 0;
    private lastNotificationMoment: number = 0;
    private readonly tryInvokeAnimCountMax: number = 1000000000;
}
