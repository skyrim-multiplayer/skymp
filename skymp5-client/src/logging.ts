import { printConsole } from "@skyrim-platform/skyrim-platform";
import { ClientListener } from "./services/services/clientListener";
import * as fs from "fs";

const logPath = "Data/Platform/Plugins/skymp5-client.log";

// Wipe the log on startup so each session starts fresh
try { fs.writeFileSync(logPath, ""); } catch (_) {}

function writeToFile(line: string) {
    try {
        fs.appendFileSync(logPath, line + "\n");
    } catch (_) {}
}

// TODO: redirect this to spdlog
export function logError(service: ClientListener | string, ...rest: unknown[]) {
    const restProcessed = rest.map(item => {
        if (item instanceof Error) {
            return item.stack || item.message;
        }
        return item;
    });

    const line = `[ERROR] ${typeof service !== "string" ? service.constructor.name : service}: ${restProcessed.join(" ")}`;
    printConsole(line);
    writeToFile(line);
}

// TODO: redirect this to spdlog
export function logTrace(service: ClientListener | string, ...rest: unknown[]) {
    const restProcessed = rest.map(item => {
        if (item instanceof Error) {
            return item.stack || item.message;
        }
        return item;
    });

    const line = `[TRACE] ${typeof service !== "string" ? service.constructor.name : service}: ${restProcessed.join(" ")}`;
    printConsole(line);
    writeToFile(line);
}
