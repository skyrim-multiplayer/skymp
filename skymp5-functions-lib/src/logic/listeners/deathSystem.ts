import { PlayerController } from "../PlayerController";
import { GameModeListener } from "./gameModeListener";
import { LocationalData, Mp, ServerSettings } from "../../types/mp";
import { ChatMessage } from "../../props/chatProperty";

export class DeathSystem implements GameModeListener {
    constructor(private mp: Mp, private controller: PlayerController) {
        this.serverSettings = this.mp.getServerSettings();

        this.startPoints = (this.serverSettings.startPoints || [])
            .filter(point => typeof point.angleZ === "number")
            .filter(point => Array.isArray(point.pos))
            .filter(point => !Number.isNaN(parseInt(point.worldOrCell)))
            .map(point => {
                const locationalData: LocationalData = {
                    cellOrWorldDesc: mp.getDescFromId(parseInt(point.worldOrCell)),
                    pos: point.pos,
                    rot: [0, 0, point.angleZ]
                }
                return locationalData;
            });
        
        if (this.startPoints.length === 0) {
            console.warn("DeathSystem: No startPoints found in server settings");
        }
    }

    onPlayerActivateObject(casterActorId: number, targetObjectDesc: string, targetActorId: number): "continue" {
        const formid = this.mp.getIdFromDesc(targetObjectDesc);
        const isTeleportDoor = this.controller.isTeleportActivator(formid);
        if (!isTeleportDoor) {
            return "continue";
        }

        const locationalData = this.mp.get(casterActorId, "locationalData");
        const newSpawnPoint = this.getNearestPoint(locationalData, this.startPoints);
        if (newSpawnPoint !== undefined) {
            this.mp.set(casterActorId, "private.spawnPointBackup", newSpawnPoint);
        }

        return "continue";
    }

    onPlayerJoin(actorId: number) {
    }

    onPlayerDeath(targetActorId: number, killerActorId?: number | undefined) {
        const locationalData = this.mp.get(targetActorId, "locationalData");
        const newSpawnPoint = this.getNearestPoint(locationalData, this.startPoints) 
            || this.mp.get(targetActorId, "private.spawnPointBackup") as LocationalData | undefined
            || this.getRandomPoint(this.startPoints);
        if (newSpawnPoint !== undefined) {
            this.mp.set(targetActorId, "spawnPoint", newSpawnPoint);
        }
        this.controller.sendChatMessage(targetActorId, ChatMessage.system("You have been respawned"));
    }

    private getDistance(posA: [number, number, number], posB: [number, number, number]) {
        const dx = posA[0] - posB[0];
        const dy = posA[1] - posB[1];
        const dz = posA[2] - posB[2];
        return Math.sqrt(dx * dx + dy * dy + dz * dz);
    }

    private getNearestPoint(startingPoint: LocationalData, candidates: LocationalData[]): LocationalData | undefined {
        const sameCellOrWorldCandidates = candidates.filter(candidate => candidate.cellOrWorldDesc === startingPoint.cellOrWorldDesc);
        
        let nearestPoint: LocationalData | undefined = undefined;
        let nearestDistance = Number.MAX_VALUE;
        for (const candidate of sameCellOrWorldCandidates) {
            const distance = this.getDistance(startingPoint.pos, candidate.pos);
            if (distance < nearestDistance) {
                nearestDistance = distance;
                nearestPoint = candidate;
            }
        }

        return nearestPoint;
    }

    private getRandomPoint(candidates: LocationalData[]): LocationalData | undefined {
        if (candidates.length === 0) {
            return undefined;
        }

        const index = Math.floor(Math.random() * candidates.length);
        return candidates[index];
    }

    private serverSettings: ServerSettings;
    private startPoints: LocationalData[];
}
