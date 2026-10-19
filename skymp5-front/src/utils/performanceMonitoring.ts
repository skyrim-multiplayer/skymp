interface FrontendMetric {
  name: string;
  value: number;
  source: string;
  ts: number;
}

interface FrontendMetricsPayload {
  source: string;
  url: string;
  path: string;
  userAgent: string;
  language: string;
  platform: string;
  visibilityState: string;
  sessionId: string;
  metrics: FrontendMetric[];
}

const MAX_BUFFER = 30;
const BUFFER_FLUSH_MS = 15000;

let metricBuffer: FrontendMetric[] = [];
let flushTimer: ReturnType<typeof setInterval> | null = null;
let metricsDisabled = false;
const metricsSessionId = `${Date.now().toString(36)}-${Math.random()
  .toString(36)
  .slice(2, 10)}`;

const pushMetric = (metric: FrontendMetric): void => {
  metricBuffer.push(metric);
  if (metricBuffer.length > MAX_BUFFER) {
    metricBuffer = metricBuffer.slice(metricBuffer.length - MAX_BUFFER);
  }
};

const resolveMetricsEndpoint = (): string => {
  const envEndpoint = (window as any)?.SKYMP_METRICS_ENDPOINT as
    | string
    | undefined;
  if (envEndpoint && typeof envEndpoint === 'string') return envEndpoint;
  return '/api/frontend/metrics';
};

const isBrowserDevUiMode = (): boolean => {
  try {
    const params = new URLSearchParams(window.location.search);
    return params.get('devUi') === '1';
  } catch {
    return false;
  }
};

const shouldSkipNetworkFlush = (endpoint: string): boolean => {
  if (!isBrowserDevUiMode()) return false;

  const explicitEndpoint = (window as any)?.SKYMP_METRICS_ENDPOINT;
  if (typeof explicitEndpoint === 'string' && explicitEndpoint.trim()) { return false; }

  return endpoint === '/api/frontend/metrics';
};

const flushMetrics = (): void => {
  if (metricBuffer.length === 0 || metricsDisabled) return;

  const payload: FrontendMetricsPayload = {
    source: 'skymp5-front',
    url: window.location.href,
    path: window.location.pathname,
    userAgent: navigator.userAgent,
    language: navigator.language,
    platform: navigator.platform,
    visibilityState: String(document.visibilityState || 'unknown'),
    sessionId: metricsSessionId,
    metrics: metricBuffer
  };

  const endpoint = resolveMetricsEndpoint();
  if (shouldSkipNetworkFlush(endpoint)) {
    metricBuffer = [];
    metricsDisabled = true;
    return;
  }

  metricBuffer = [];

  try {
    const encoded = JSON.stringify(payload);
    if (navigator.sendBeacon) {
      const blob = new Blob([encoded], { type: 'application/json' });
      navigator.sendBeacon(endpoint, blob);
      return;
    }

    fetch(endpoint, {
      method: 'POST',
      headers: {
        'Content-Type': 'application/json'
      },
      body: encoded,
      keepalive: true
    }).catch(() => {
      // Metrics must never break UI runtime.
    });
  } catch {
    // Metrics must never break UI runtime.
  }
};

const initPerformanceObservers = (): void => {
  if (typeof PerformanceObserver === 'undefined') return;

  try {
    const observer = new PerformanceObserver((entryList) => {
      entryList.getEntries().forEach((entry) => {
        pushMetric({
          name: entry.name,
          value: Number(entry.duration || 0),
          source: entry.entryType,
          ts: Date.now()
        });
      });
    });

    observer.observe({
      entryTypes: [
        'navigation',
        'resource',
        'paint',
        'largest-contentful-paint'
      ] as any
    });
  } catch {
    // Some browsers may not support all entry types.
  }
};

const initErrorTracking = (): void => {
  window.addEventListener('error', (event) => {
    pushMetric({
      name: event.message || 'error',
      value: 1,
      source: 'window.error',
      ts: Date.now()
    });
  });

  window.addEventListener('unhandledrejection', () => {
    pushMetric({
      name: 'unhandledrejection',
      value: 1,
      source: 'window.unhandledrejection',
      ts: Date.now()
    });
  });
};

export const initPerformanceMonitoring = (): void => {
  if (flushTimer) return;

  initPerformanceObservers();
  initErrorTracking();

  flushTimer = setInterval(flushMetrics, BUFFER_FLUSH_MS);
  window.addEventListener('beforeunload', flushMetrics);
};
