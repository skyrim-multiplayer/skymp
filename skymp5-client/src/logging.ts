import { printConsole } from "@skyrim-platform/skyrim-platform";
import { ClientListener } from "./services/services/clientListener";

// TODO: redirect this to spdlog
export function logError(service: ClientListener | string, ...rest: unknown[]) {
    printConsole(`Error in ${typeof service !== "string" ? service.constructor.name : service}:`, ...rest);
}

// TODO: redirect this to spdlog
export function logTrace(service: ClientListener | string, ...rest: unknown[]) {
    printConsole(`Trace in ${typeof service !== "string" ? service.constructor.name : service}:`, ...rest);
}
