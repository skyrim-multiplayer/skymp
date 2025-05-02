import { logTrace } from "../../logging";
import { ClientListener, CombinedController, Sp } from "./clientListener";
import { Session } from 'inspector';
import * as fs from "fs";

export class ProfilingService extends ClientListener {
    constructor(private sp: Sp, private controller: CombinedController) {
        super();
        const settings = sp.settings["skymp5-client"];

        if (!settings["enableProfiling"]) {
            logTrace(this, "ProfilingService: disabled");
            return;
        }

        const profilingDurationMs = this.getInteger(settings["profilingDurationMs"]) || 10000;

        const session = new Session();
        session.connect();

        logTrace(this, "ProfilingService: start");

        this.startProfiling(session, profilingDurationMs);
    }

    private async startProfiling(session: Session, profilingDurationMs: number) {
        await new Promise((resolve, reject) => {
            session.post('Profiler.enable', (err: Error | null) => {
                if (err) {
                    reject(err);
                }
                else {
                    resolve(undefined);
                }
            });
        });

        await new Promise((resolve, reject) => {
            session.post('Profiler.start', (err: Error | null) => {
                if (err) {
                    reject(err);
                }
                else {
                    resolve(undefined);
                }
            });
        });

        await new Promise((resolve) => {
            setTimeout(resolve, profilingDurationMs);
        });

        logTrace(this, "ProfilingService: stop");

        const { profile } = await new Promise<any>((resolve, reject) => {
            session.post('Profiler.stop', (err: Error | null, res) => {
                if (err) {
                    reject(err);
                } else {
                    resolve(res);
                }
            });
        });

        const fileNameSuffix = Math.random().toString().replace(".0", "");
        fs.writeFileSync(`./profile${fileNameSuffix}.cpuprofile`, JSON.stringify(profile));
    }

    private getInteger(value: unknown) {
        if (typeof value === 'number') {
            if (Number.isInteger(value)) {
                return value;
            }
        }
        return undefined;
    }
}
