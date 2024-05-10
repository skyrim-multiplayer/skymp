import { printConsole } from "@skyrim-platform/skyrim-platform";
import { ClientListener } from "./services/services/clientListener";

// TODO: redirect this to spdlog
export function logError(service: ClientListener, ...rest: unknown[]) {
    printConsole(`Error in ${service.constructor.name}:`, ...rest);
}

// TODO: redirect this to spdlog
export function logTrace(service: ClientListener, ...rest: unknown[]) {
    printConsole(`Trace in ${service.constructor.name}:`, ...rest);
}
