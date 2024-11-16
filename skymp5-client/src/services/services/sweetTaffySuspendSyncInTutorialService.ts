import { logError, logTrace } from "src/logging";
import { QuerySuspendSyncEvent } from "../events/querySuspendSync";
import { ClientListener, Sp, CombinedController } from "./clientListener";
import { ObjectReferenceEx } from "src/extensions/objectReferenceEx";

interface SuspendZonePoint {
    pos: number[];
    radius: number;
    worldOrCell: string;
}

interface SuspendZoneSettings {
    points: SuspendZonePoint[];
    keywordImmuneNoSyncZone?: string;
}

export class SweetTaffySuspendSyncInTutorialService extends ClientListener {
    constructor(private sp: Sp, private controller: CombinedController) {
        super();

        if (!this.hasSweetPie()) {
            logTrace(this, "SweetTaffy features disabled");
            return;
        }

        logTrace(this, "SweetTaffy features enabled");

        controller.emitter.on("querySuspendSync", (e) => this.onQuerySuspendSync(e));
    }

    private onQuerySuspendSync(e: QuerySuspendSyncEvent) {
        // See also sweetTaffySuspendSyncInTutorialService.ts
        // const suspendZoneSettings = this.getSettingsFromFile();
        const suspendZoneSettings = this.getSettingsDefault();

        if (!suspendZoneSettings) {
            return;
        }

        const { points, keywordImmuneNoSyncZone } = suspendZoneSettings;
        const suspendNeeded = this.isSyncSuspendNeeded(points, keywordImmuneNoSyncZone)

        if (suspendNeeded) {
            e.suspend();
        }
    }

    private isSyncSuspendNeeded(points: SuspendZonePoint[], keywordImmuneNoSyncZone?: string) {
        // TODO: de-duplicate implementation with Green Zone implementation in gamemode
        let player = this.sp.Game.getPlayer()!;

        let isNoSyncZone = false;

        let pos = [
            player.getPositionX(),
            player.getPositionY(),
            player.getPositionZ(),
        ];

        const worldOrCell = ObjectReferenceEx.getWorldOrCell(player);

        for (let point of points) {
            if (parseInt(point.worldOrCell) !== worldOrCell) {
                continue;
            }

            let distance = Math.sqrt(
                Math.pow(pos[0] - point.pos[0], 2) +
                Math.pow(pos[1] - point.pos[1], 2) +
                Math.pow(pos[2] - point.pos[2], 2)
            );
            if (distance < point.radius) {
                isNoSyncZone = true;
                break;
            }
        }

        const isImmune = !keywordImmuneNoSyncZone || !player.wornHasKeyword(this.sp.Keyword.getKeyword(keywordImmuneNoSyncZone));

        if (isImmune !== this.wasImmune) {
            this.wasImmune = isImmune;
            logTrace(this, "Immune to NoSync zone:", isImmune);
        }

        if (isNoSyncZone !== this.wasInNoSyncZone) {
            this.wasInNoSyncZone = isNoSyncZone;
            logTrace(this, "NoSync zone:", isNoSyncZone);
        }

        return isNoSyncZone && !isImmune;
    }

    private getSettingsFromFile(): SuspendZoneSettings | null {
        const sweetTaffySuspendSyncInTutorialService = this.sp.settings["skymp5-client"]["sweetTaffySuspendSyncInTutorialService"];

        if (!sweetTaffySuspendSyncInTutorialService || typeof sweetTaffySuspendSyncInTutorialService !== "object") {
            logError(this, `No sweetTaffySuspendSyncInTutorialService settings found`);
            return null;
        }

        const suspendZoneSettings = (sweetTaffySuspendSyncInTutorialService as Record<string, unknown>).suspendZoneSettings;
        if (!suspendZoneSettings || typeof suspendZoneSettings !== "object") {
            logError(this, `No suspendZoneSettings settings found`);
            return null;
        }

        return suspendZoneSettings as SuspendZoneSettings;
    }

    private getSettingsDefault(): SuspendZoneSettings | null {
        return {
            points: [],
            keywordImmuneNoSyncZone: "",
        };
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

    private wasInNoSyncZone = false;
    private wasImmune = false;
}
