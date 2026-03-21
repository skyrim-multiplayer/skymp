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

export const cppMetricsErrorsCounter = new promClient.Counter({
  name: 'skymp_cpp_metrics_errors_total',
  help: 'Total number of errors during C++ metrics collection',
});

export const tickDurationHistogram = new promClient.Histogram({
  name: "skymp_tick_duration_seconds",
  help: "Duration of tick handling in seconds",
  buckets: [0.001, 0.005, 0.01, 0.05, 0.1, 0.5, 1],
});

export const tickDurationSummary = new promClient.Summary({
  name: "skymp_tick_duration_summary_seconds",
  help: "Duration of tick handling in seconds",
  percentiles: [0.5, 0.9, 0.95, 0.99, 0.995, 0.999],
});

export const getAggregatedMetrics = async (scampServer?: any): Promise<string> => {
  const tsStart = performance.now();
  let metrics = '# === JS metrics begin ===\n' + await register.metrics() + '\n';
  const tsJsCollected = performance.now();

  try {
    const cppMetrics: string = scampServer?.getPrometheusMetrics() ?? "";
    metrics += '# === CPP metrics begin ===\n' + cppMetrics;
  } catch (err) {
    console.error("Failed to collect native metrics:", err);
    cppMetricsErrorsCounter.inc();
  }
  const tsCppCollected = performance.now();

  console.log('Metrics collection timings (ms):', Math.ceil(tsJsCollected - tsStart), Math.ceil(tsCppCollected - tsJsCollected));

  return metrics;
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
