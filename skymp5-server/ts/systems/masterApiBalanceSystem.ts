import { System, Log } from "./system";
import Axios from "axios";
import { SystemContext } from "./system";
import { getMyPublicIp } from "../publicIp";

export class MasterApiBalanceSystem implements System {
    systemName = "MasterApiBalanceSystem";
      
    constructor(
        private log: Log,
        private maxPlayers: number,
        private masterUrl: string | null,
        private serverPort: number,
        private ip: string,
        private offlineMode: boolean) { 
            this.sessionByUserId = new Array<string | undefined>(this.maxPlayers);
        }

    async initAsync(ctx: SystemContext): Promise<void> {
        const listenerFn = (userId: number, session: string) => {
            console.log(`MasterApiBalanceSystem.userAssignSession - Assigning session for userId ${userId}`);
            this.sessionByUserId[userId] = session;
        };
        ctx.gm.on("userAssignSession", listenerFn);

        if (this.ip && this.ip != "null") {
            this.myAddr = this.ip + ":" + this.serverPort;
        } else {
            this.myAddr = (await getMyPublicIp()) + ":" + this.serverPort;
        }
        this.log(
            `MasterApiBalanceSystem system assumed that ${this.myAddr} is our address on master`
        );

        // Effectively makes mp.getUserBalance a part of gamemode API
        (ctx.svr as any).getUserBalance = async (userId: number): Promise<number> => {
            if (this.offlineMode) {
                console.log("MasterApiBalanceSystem.getUserBalance - Always zero balance in offline mode");
                return 0;
            }

            const session = this.sessionByUserId[userId];
            if (!session) {
                console.error(`MasterApiBalanceSystem.getUserBalance - Invalid session value for userId ${userId} (session = ${session})`);
                throw new Error(`MasterApiBalanceSystem.getUserBalance - Invalid session value for userId ${userId} (session = ${session})`);
            }
            return await this.getUserBalanceImpl(session);
        };
    }

    private async getUserBalanceImpl(session: string): Promise<number> {
        try {
            const response = await Axios.get(
                `${this.masterUrl}/api/servers/${this.myAddr}/sessions/${session}/balance`
            );
            if (!response.data || !response.data.user || !response.data.user.id || typeof response.data.user.balance !== "number") {
                throw new Error(`getUserBalance: bad master-api response ${JSON.stringify(response.data)}`);
            }
            return response.data.user.balance as number;
        } catch (error) {
            throw error;
        }
    }

    connect(userId: number, ctx: SystemContext) {
        console.log(`MasterApiBalanceSystem.connect - Cleaning session for userId ${userId}`);
        this.sessionByUserId[userId] = undefined;
    }

    disconnect(userId: number, ctx: SystemContext) {
        console.log(`MasterApiBalanceSystem.disconnect - Cleaning session for userId ${userId}`);
        this.sessionByUserId[userId] = undefined;
    }

    private myAddr: string;
    private sessionByUserId: Array<string | undefined>;
}
