import * as promClient from "prom-client";
import { System, SystemContext } from "./system";

promClient.collectDefaultMetrics();

export const register = promClient.register;

export const connectsCounter = new promClient.Counter({
  name: "skymp_connects_total",
  help: "Total number of player connections",
});

export const disconnectsCounter = new promClient.Counter({
  name: "skymp_disconnects_total",
  help: "Total number of player disconnections",
});

export const loginsCounter = new promClient.Counter({
  name: "skymp_logins_total",
  help: "Total number of successful logins",
});

export const loginErrorsCounter = new promClient.Counter({
  name: "skymp_login_errors_total",
  help: "Total number of login errors",
  labelNames: ["reason"] as const,
});

export const rpcCallsCounter = new promClient.Counter({
  name: "skymp_rpc_calls_total",
  help: "Total number of RPC calls received",
  labelNames: ["rpcClassName"] as const,
});

export const rpcDurationHistogram = new promClient.Histogram({
  name: "skymp_rpc_duration_seconds",
  help: "Duration of RPC call handling in seconds",
  labelNames: ["rpcClassName"] as const,
  buckets: [0.001, 0.005, 0.01, 0.05, 0.1, 0.5, 1],
});

export const getAggregatedMetrics = async (scampServer?: any): Promise<string> => {
  const jsMetrics = await register.metrics();

  if (!scampServer) {
    return jsMetrics;
  }

  try {
    const cppMetrics: string = scampServer.getPrometheusMetrics();
    if (!cppMetrics) {
      return jsMetrics;
    }
    return jsMetrics.trimEnd() + "\n" + cppMetrics.trimEnd() + "\n";
  } catch (err) {
    console.error("Failed to collect native metrics:", err);
    return jsMetrics;
  }
};

export class MetricsSystem implements System {
  systemName = "MetricsSystem";

  connect(_userId: number, _ctx: SystemContext): void {
    connectsCounter.inc();
  }

  disconnect(_userId: number, _ctx: SystemContext): void {
    disconnectsCounter.inc();
  }
}
