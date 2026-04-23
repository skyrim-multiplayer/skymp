import { useCallback, useEffect, useMemo, useRef, useState } from 'react';

import {
  getConfig,
  getDoctor,
  getMeta,
  getStatus,
  runSyncRequest,
  saveConfig,
} from './api.js';
import { useBuildMonitor } from './hooks/useBuildMonitor.js';
import { useModsMonitor } from './hooks/useModsMonitor.js';
import { useRuntimeMonitor } from './hooks/useRuntimeMonitor.js';

const TABS = [
  { id: 'overview', label: 'Overview', icon: 'home' },
  { id: 'builds', label: 'Builds', icon: 'builds' },
  { id: 'mods', label: 'Mods', icon: 'tag' },
  { id: 'sync', label: 'Sync', icon: 'sync' },
  { id: 'doctor', label: 'Doctor', icon: 'doctor' },
  { id: 'settings', label: 'Settings', icon: 'settings' },
];

const TAB_SUBTITLES = {
  overview: 'A friendlier dashboard for build status, doctor checks, sync actions, and local configuration.',
  builds: 'Run and manage SkyMP build profiles, monitor queue and status, and inspect results.',
  mods: 'Inventory the current Data directory, inspect plugin state, and toggle plugin-backed mods through Plugins.txt.',
  sync: 'Review artifact health, compare source and destination state, and run sync actions when needed.',
  doctor: 'Inspect environment issues for local tooling, paths, and runtime assumptions.',
  settings: 'Manage the local build-assistant configuration used by this machine.',
};

const PROFILE_VISUALS = {
  cmake: {
    accent: '#3b82f6',
    soft: 'rgba(59, 130, 246, 0.18)',
    border: 'rgba(59, 130, 246, 0.34)',
    icon: 'cube',
  },
  workflow: {
    accent: '#22c55e',
    soft: 'rgba(34, 197, 94, 0.18)',
    border: 'rgba(34, 197, 94, 0.32)',
    icon: 'sparkles',
  },
  node: {
    accent: '#a855f7',
    soft: 'rgba(168, 85, 247, 0.18)',
    border: 'rgba(168, 85, 247, 0.32)',
    icon: 'code',
  },
  assistant: {
    accent: '#38bdf8',
    soft: 'rgba(56, 189, 248, 0.18)',
    border: 'rgba(56, 189, 248, 0.32)',
    icon: 'monitor',
  },
  advanced: {
    accent: '#f59e0b',
    soft: 'rgba(245, 158, 11, 0.18)',
    border: 'rgba(245, 158, 11, 0.32)',
    icon: 'sliders',
  },
  default: {
    accent: '#64748b',
    soft: 'rgba(100, 116, 139, 0.18)',
    border: 'rgba(148, 163, 184, 0.28)',
    icon: 'cube',
  },
};

function summarizeDoctorIssues(issues) {
  return issues.reduce(
    (summary, issue) => {
      summary.total += 1;
      summary[issue.level] += 1;
      return summary;
    },
    { total: 0, error: 0, warn: 0, info: 0 },
  );
}

function statusTone(status) {
  if (status === 'ok') {
    return 'ok';
  }
  if (status === 'dirty') {
    return 'dirty';
  }
  if (status === 'skipped' || status === 'skipped_no_skyrim') {
    return 'warn';
  }
  return 'bad';
}

function buildTone(status) {
  if (status === 'succeeded' || status === 'ready') {
    return 'ok';
  }
  if (status === 'running') {
    return 'info';
  }
  if (status === 'queued' || status === 'pending') {
    return 'neutral';
  }
  if (status === 'cancelled' || status === 'idle') {
    return 'warn';
  }
  if (status === 'blocked') {
    return 'warn';
  }
  return 'bad';
}

function runtimeTone(state) {
  if (state === 'ready_to_launch') {
    return 'ok';
  }
  if (state === 'running') {
    return 'info';
  }
  if (state === 'skyrim_root_missing' || state === 'skse_missing') {
    return 'warn';
  }
  if (state === 'unsupported') {
    return 'neutral';
  }
  return 'bad';
}

function modStateTone(state) {
  if (state === 'enabled') {
    return 'ok';
  }
  if (state === 'disabled') {
    return 'warn';
  }
  if (state === 'inventory_only') {
    return 'neutral';
  }
  return 'bad';
}

function modKindLabel(kind) {
  switch (kind) {
    case 'plugin_backed':
      return 'Plugin-backed';
    case 'platform_plugin':
      return 'Platform plugin';
    case 'platform_dev_plugin':
      return 'PluginsDev plugin';
    case 'skse_plugin':
      return 'SKSE plugin';
    case 'folder_group':
      return 'Folder group';
    case 'missing_plugin':
      return 'Missing plugin';
    default:
      return humanizeLabel(kind);
  }
}

function formatDateTime(value) {
  if (!value) {
    return 'Not yet';
  }
  const date = new Date(value);
  if (Number.isNaN(date.getTime())) {
    return value;
  }
  return date.toLocaleString();
}

function formatDuration(startedAt, endedAt) {
  if (!startedAt) {
    return 'Not started';
  }
  const startMs = new Date(startedAt).getTime();
  const endMs = endedAt ? new Date(endedAt).getTime() : Date.now();
  if (Number.isNaN(startMs) || Number.isNaN(endMs)) {
    return 'Unknown';
  }

  const totalSeconds = Math.max(0, Math.round((endMs - startMs) / 1000));
  if (totalSeconds < 60) {
    return `${totalSeconds}s`;
  }

  const minutes = Math.floor(totalSeconds / 60);
  const seconds = totalSeconds % 60;
  if (minutes < 60) {
    return `${minutes}m ${seconds}s`;
  }

  const hours = Math.floor(minutes / 60);
  const remainingMinutes = minutes % 60;
  return `${hours}h ${remainingMinutes}m`;
}

function buildStateLabel(snapshot) {
  const activeState = snapshot?.summary?.activeState ?? 'idle';
  if (activeState !== 'idle') {
    return activeState;
  }
  return snapshot?.summary?.lastFinishedStatus ?? 'idle';
}

function humanizeLabel(value) {
  if (!value) {
    return 'Unknown';
  }
  const label = String(value).replace(/_/g, ' ');
  return label.charAt(0).toUpperCase() + label.slice(1);
}

function formatCompactDateTime(value) {
  if (!value) {
    return 'Never';
  }
  const date = new Date(value);
  if (Number.isNaN(date.getTime())) {
    return value;
  }
  return date.toLocaleString([], {
    month: 'numeric',
    day: 'numeric',
    year: 'numeric',
    hour: 'numeric',
    minute: '2-digit',
  });
}

function formatStartedLabel(value) {
  if (!value) {
    return 'Not started';
  }
  const date = new Date(value);
  if (Number.isNaN(date.getTime())) {
    return value;
  }
  const now = new Date();
  if (date.toDateString() === now.toDateString()) {
    return date.toLocaleTimeString([], {
      hour: 'numeric',
      minute: '2-digit',
    });
  }
  return formatCompactDateTime(value);
}

function formatLastUpdatedLabel(value) {
  if (!value) {
    return 'Last updated recently';
  }
  const timestamp = typeof value === 'number' ? value : new Date(value).getTime();
  if (Number.isNaN(timestamp)) {
    return 'Last updated recently';
  }

  const diffSeconds = Math.max(0, Math.round((Date.now() - timestamp) / 1000));
  if (diffSeconds < 10) {
    return 'Last updated just now';
  }
  if (diffSeconds < 60) {
    return `Last updated ${diffSeconds}s ago`;
  }

  const diffMinutes = Math.round(diffSeconds / 60);
  if (diffMinutes < 60) {
    return `Last updated ${diffMinutes}m ago`;
  }

  return `Last updated ${formatStartedLabel(timestamp)}`;
}

function getProfileVisual(profile) {
  if (!profile?.category) {
    return PROFILE_VISUALS.default;
  }
  return PROFILE_VISUALS[profile.category] ?? PROFILE_VISUALS.default;
}

function SymbolIcon({ name, className = '' }) {
  const props = {
    className,
    viewBox: '0 0 24 24',
    fill: 'none',
    stroke: 'currentColor',
    strokeWidth: '1.8',
    strokeLinecap: 'round',
    strokeLinejoin: 'round',
    'aria-hidden': 'true',
  };

  switch (name) {
    case 'home':
      return (
        <svg {...props}>
          <path d="M3.5 10.5 12 3.75l8.5 6.75" />
          <path d="M6.25 9.75V20.25H17.75V9.75" />
        </svg>
      );
    case 'builds':
      return (
        <svg {...props}>
          <path d="M7 6.5h10" />
          <path d="M12 6.5V3.75" />
          <path d="m8.75 10.25-3.5 3.5 5.25 5.25 3.5-3.5" />
          <path d="m13.5 8.5 2-2a2.12 2.12 0 0 1 3 3l-2 2" />
        </svg>
      );
    case 'sync':
      return (
        <svg {...props}>
          <path d="M7.5 7.5h10l-2.5-2.5" />
          <path d="m17.5 7.5-2.5 2.5" />
          <path d="M16.5 16.5H6.5L9 19" />
          <path d="m6.5 16.5 2.5-2.5" />
        </svg>
      );
    case 'doctor':
      return (
        <svg {...props}>
          <path d="M12 3.5c2.75 2.2 5.5 3.3 8.25 3.5v4.5c0 5.25-3.4 8.2-8.25 9.75-4.85-1.55-8.25-4.5-8.25-9.75V7c2.75-.2 5.5-1.3 8.25-3.5Z" />
          <path d="m8.75 12.5 2.25 2.25 4.5-4.75" />
        </svg>
      );
    case 'settings':
      return (
        <svg {...props}>
          <circle cx="12" cy="12" r="3" />
          <path d="M12 3.5v2.25" />
          <path d="M12 18.25v2.25" />
          <path d="m5.95 5.95 1.6 1.6" />
          <path d="m16.45 16.45 1.6 1.6" />
          <path d="M3.5 12h2.25" />
          <path d="M18.25 12h2.25" />
          <path d="m5.95 18.05 1.6-1.6" />
          <path d="m16.45 7.55 1.6-1.6" />
        </svg>
      );
    case 'cube':
      return (
        <svg {...props}>
          <path d="m12 3.75 7.5 4.25V16L12 20.25 4.5 16V8Z" />
          <path d="M12 20.25V12" />
          <path d="m19.5 8-7.5 4-7.5-4" />
        </svg>
      );
    case 'sparkles':
      return (
        <svg {...props}>
          <path d="m12 4 1.75 4.25L18 10l-4.25 1.75L12 16l-1.75-4.25L6 10l4.25-1.75Z" />
          <path d="m18.5 4 .6 1.4 1.4.6-1.4.6-.6 1.4-.6-1.4-1.4-.6 1.4-.6Z" />
          <path d="m5.5 15.5.75 1.75 1.75.75-1.75.75-.75 1.75-.75-1.75-1.75-.75 1.75-.75Z" />
        </svg>
      );
    case 'code':
      return (
        <svg {...props}>
          <path d="m9 7.5-4 4.5 4 4.5" />
          <path d="m15 7.5 4 4.5-4 4.5" />
          <path d="m13.5 5.5-3 13" />
        </svg>
      );
    case 'monitor':
      return (
        <svg {...props}>
          <rect x="4" y="5" width="16" height="11" rx="2.5" />
          <path d="M9.5 20h5" />
          <path d="M12 16v4" />
        </svg>
      );
    case 'sliders':
      return (
        <svg {...props}>
          <path d="M5 6.5h14" />
          <path d="M5 12h14" />
          <path d="M5 17.5h14" />
          <circle cx="9" cy="6.5" r="1.5" />
          <circle cx="15" cy="12" r="1.5" />
          <circle cx="11" cy="17.5" r="1.5" />
        </svg>
      );
    case 'search':
      return (
        <svg {...props}>
          <circle cx="11" cy="11" r="5.75" />
          <path d="m18 18 2.5 2.5" />
        </svg>
      );
    case 'filter':
      return (
        <svg {...props}>
          <path d="M4.5 6h15l-6 6.75v4.25l-3 1.5v-5.75Z" />
        </svg>
      );
    case 'play':
      return (
        <svg {...props}>
          <path d="m9 7 8 5-8 5Z" />
        </svg>
      );
    case 'terminal':
      return (
        <svg {...props}>
          <rect x="4" y="5" width="16" height="14" rx="2.25" />
          <path d="m8 10 2.5 2.5L8 15" />
          <path d="M12.5 15h3.5" />
        </svg>
      );
    case 'info':
      return (
        <svg {...props}>
          <circle cx="12" cy="12" r="8" />
          <path d="M12 10v5" />
          <path d="M12 7.25h.01" />
        </svg>
      );
    case 'tag':
      return (
        <svg {...props}>
          <path d="m20 12-8 8-8-8V6.5A2.5 2.5 0 0 1 6.5 4H12Z" />
          <circle cx="8" cy="8" r="1.25" />
        </svg>
      );
    case 'link':
      return (
        <svg {...props}>
          <path d="M9.5 14.5 14.5 9.5" />
          <path d="m7.25 16.75-1.5 1.5a3 3 0 1 1-4.25-4.25l3.25-3.25A3 3 0 0 1 9 10" />
          <path d="m16.75 7.25 1.5-1.5a3 3 0 0 0-4.25-4.25L10.75 4.75A3 3 0 0 0 10 9" />
        </svg>
      );
    case 'document':
      return (
        <svg {...props}>
          <path d="M7 3.75h6l4 4v12.5H7Z" />
          <path d="M13 3.75v4h4" />
        </svg>
      );
    case 'folder':
      return (
        <svg {...props}>
          <path d="M4.5 7.25h5l1.75 2H19.5v7.5a2 2 0 0 1-2 2h-11a2 2 0 0 1-2-2Z" />
          <path d="M4.5 7.25v-1a2 2 0 0 1 2-2h3l1.75 2H17.5a2 2 0 0 1 2 2v1" />
        </svg>
      );
    case 'grip-vertical':
      return (
        <svg {...props}>
          <path d="M9 6.5h.01" />
          <path d="M9 12h.01" />
          <path d="M9 17.5h.01" />
          <path d="M15 6.5h.01" />
          <path d="M15 12h.01" />
          <path d="M15 17.5h.01" />
        </svg>
      );
    case 'queue':
      return (
        <svg {...props}>
          <path d="M6.5 6.5h11" />
          <path d="M6.5 11.5h11" />
          <path d="M6.5 16.5h11" />
          <path d="m4.5 6.5 1.5 1.5 2.25-2.25" />
          <path d="m4.5 11.5 1.5 1.5 2.25-2.25" />
          <path d="m4.5 16.5 1.5 1.5 2.25-2.25" />
        </svg>
      );
    case 'refresh':
      return (
        <svg {...props}>
          <path d="M20 11a8 8 0 0 0-14-4.75" />
          <path d="M4.5 6V3.75H6.75" />
          <path d="M4 13a8 8 0 0 0 14 4.75" />
          <path d="M19.5 18v2.25h-2.25" />
        </svg>
      );
    case 'check-circle':
      return (
        <svg {...props}>
          <circle cx="12" cy="12" r="8" />
          <path d="m8.75 12.25 2.25 2.25 4.5-4.5" />
        </svg>
      );
    case 'activity':
      return (
        <svg {...props}>
          <path d="M3.75 12h4l2.25-4.5 3.5 9 2.25-4.5h4.5" />
        </svg>
      );
    case 'clock':
      return (
        <svg {...props}>
          <circle cx="12" cy="12" r="8" />
          <path d="M12 7.75v4.5l3 1.75" />
        </svg>
      );
    case 'x':
      return (
        <svg {...props}>
          <path d="m7 7 10 10" />
          <path d="M17 7 7 17" />
        </svg>
      );
    default:
      return (
        <svg {...props}>
          <circle cx="12" cy="12" r="8" />
        </svg>
      );
  }
}

function SyncSummary({ summary }) {
  const counts = summary?.byStatus ?? {};
  return (
    <div className="summary-grid">
      <StatCard label="Artifacts" value={summary?.total ?? 0} tone="neutral" />
      <StatCard label="Dirty" value={counts.dirty ?? 0} tone="dirty" />
      <StatCard label="Healthy" value={counts.ok ?? 0} tone="ok" />
      <StatCard
        label="Needs config"
        value={counts.skipped_no_skyrim ?? 0}
        tone="warn"
      />
    </div>
  );
}

function DoctorSummary({ issues }) {
  const summary = summarizeDoctorIssues(issues ?? []);
  return (
    <div className="summary-grid">
      <StatCard label="Doctor issues" value={summary.total} tone="neutral" />
      <StatCard label="Errors" value={summary.error} tone="bad" />
      <StatCard label="Warnings" value={summary.warn} tone="warn" />
      <StatCard label="Info" value={summary.info} tone="ok" />
    </div>
  );
}

function BuildSummary({ snapshot, profiles }) {
  const readyProfiles = (profiles ?? []).filter((profile) => profile.enabled).length;
  return (
    <div className="summary-grid">
      <StatCard label="Profiles" value={profiles?.length ?? 0} tone="neutral" />
      <StatCard label="Ready" value={readyProfiles} tone="ok" />
      <StatCard label="Queued" value={snapshot?.queuedJobIds?.length ?? 0} tone="warn" />
      <StatCard label="Build state" value={buildStateLabel(snapshot)} tone={buildTone(buildStateLabel(snapshot))} />
    </div>
  );
}

function RuntimeSummary({ runtime }) {
  return (
    <div className="summary-grid">
      <StatCard
        label="Runtime"
        value={humanizeLabel(runtime?.state ?? 'unknown')}
        tone={runtimeTone(runtime?.state)}
      />
      <StatCard
        label="Processes"
        value={runtime?.processCount ?? 0}
        tone={runtime?.running ? 'info' : 'neutral'}
      />
      <StatCard
        label="Launch ready"
        value={runtime?.canLaunch ? 'Yes' : 'No'}
        tone={runtime?.canLaunch ? 'ok' : runtimeTone(runtime?.state)}
      />
      <StatCard
        label="SKSE loader"
        value={runtime?.skseLoaderExists ? 'Found' : 'Missing'}
        tone={runtime?.skseLoaderExists ? 'ok' : 'warn'}
      />
    </div>
  );
}

function ModsSummary({ snapshot }) {
  const summary = snapshot?.summary ?? {
    total: 0,
    enabled: 0,
    disabled: 0,
    attention: 0,
  };

  return (
    <div className="summary-grid">
      <StatCard label="Mod entries" value={summary.total} tone="neutral" />
      <StatCard label="Enabled" value={summary.enabled} tone="ok" />
      <StatCard label="Disabled" value={summary.disabled} tone="warn" />
      <StatCard label="Attention" value={summary.attention} tone={summary.attention > 0 ? 'bad' : 'neutral'} />
    </div>
  );
}

function StatCard({ label, value, tone }) {
  return (
    <article className={`stat-card stat-card--${tone}`}>
      <span className="stat-card__label">{label}</span>
      <strong className="stat-card__value">{value}</strong>
    </article>
  );
}

function BuildMetricCard({ icon, label, value, secondaryValue, meta, tone = 'neutral' }) {
  return (
    <article className={`build-metric-card build-metric-card--${tone}`}>
      <div className="build-metric-card__header">
        <span className="build-metric-card__label">{label}</span>
        <span className="build-metric-card__icon">
          <SymbolIcon name={icon} />
        </span>
      </div>
      <div className="build-metric-card__value-row">
        <strong className="build-metric-card__value">{value}</strong>
        {secondaryValue ? <span className="build-metric-card__secondary">{secondaryValue}</span> : null}
      </div>
      <p className="build-metric-card__meta">{meta}</p>
    </article>
  );
}

function RuntimePanel({
  runtime,
  error,
  launching,
  openingFolder,
  stopping,
  refreshing,
  compact = false,
  onLaunch,
  onOpenFolder,
  onStop,
  onOpenSettings,
}) {
  const statusLabel = launching
    ? 'Launching'
    : stopping
      ? 'Stopping'
      : humanizeLabel(runtime?.state ?? 'unknown');
  const statusTone = launching || stopping ? 'info' : runtimeTone(runtime?.state);
  const detailRows = [
    { label: 'Processes', value: String(runtime?.processCount ?? 0) },
    { label: 'Skyrim root', value: formatPath(runtime?.skyrimRoot) },
    { label: 'SKSE loader', value: formatPath(runtime?.skseLoaderPath) },
    { label: 'Last checked', value: runtime?.checkedAt ? formatStartedLabel(runtime.checkedAt) : 'Unknown' },
  ];

  return (
    <section className={`panel ${compact ? 'build-sidebar-panel runtime-panel--compact' : 'runtime-panel'}`}>
      <div className="panel__header panel__header--wrap">
        <div>
          <h2>Runtime control</h2>
          <p>Launch Skyrim through SKSE, track whether it is already running, and force-stop it when needed.</p>
        </div>
        <span className={`pill pill--${statusTone}`}>{statusLabel}</span>
      </div>

      <div className={`runtime-state-card runtime-state-card--${statusTone}`}>
        <div className="runtime-state-card__icon">
          <SymbolIcon name={runtime?.running ? 'activity' : 'monitor'} />
        </div>
        <div className="runtime-state-card__copy">
          <strong>{statusLabel}</strong>
          <p>{runtime?.message || 'Runtime status is unavailable right now.'}</p>
        </div>
      </div>

      {error ? (
        <div className="runtime-panel__error">
          <strong>Runtime API error</strong>
          <p>{error}</p>
        </div>
      ) : null}

      <div className="runtime-panel__details">
        {detailRows.map((row) => (
          <div className="key-value" key={row.label}>
            <span>{row.label}</span>
            <strong>{row.value}</strong>
          </div>
        ))}
      </div>

      {runtime?.processIds?.length ? (
        <div className="runtime-panel__processes">
          <span>Detected PIDs</span>
          <code>{runtime.processIds.join(', ')}</code>
        </div>
      ) : null}

      <div className="runtime-panel__footer">
        <span>{refreshing ? 'Refreshing runtime status...' : formatLastUpdatedLabel(runtime?.checkedAt)}</span>
        <div className="button-row runtime-panel__actions">
          <button
            type="button"
            className="primary-button"
            disabled={launching || stopping || !runtime?.canLaunch}
            onClick={() => void onLaunch()}
          >
            <SymbolIcon name="play" className="button-icon" />
            <span>{launching ? 'Launching...' : 'Launch Skyrim'}</span>
          </button>
          <button
            type="button"
            className="ghost-button"
            disabled={launching || openingFolder || stopping || !runtime?.canStop}
            onClick={() => void onStop()}
          >
            <SymbolIcon name="x" className="button-icon" />
            <span>{stopping ? 'Stopping...' : 'Force stop'}</span>
          </button>
          <button
            type="button"
            className="ghost-button"
            disabled={launching || openingFolder || stopping || !runtime?.canOpenFolder}
            onClick={() => void onOpenFolder()}
          >
            <SymbolIcon name="folder" className="button-icon" />
            <span>{openingFolder ? 'Opening...' : 'Open game folder'}</span>
          </button>
          {!compact ? (
            <button type="button" className="ghost-button" onClick={onOpenSettings}>
              Settings
            </button>
          ) : null}
        </div>
      </div>
    </section>
  );
}

function ModsInventoryTableSection({
  title,
  description,
  mods,
  togglingModId,
  reordering,
  draggable = false,
  onReorder,
  onToggle,
}) {
  const [draggedModId, setDraggedModId] = useState('');
  const [dragTargetModId, setDragTargetModId] = useState('');

  if (mods.length === 0) {
    return null;
  }

  const canReorder = draggable && reordering !== true && !togglingModId;

  function handleDragStart(event, modId) {
    if (!canReorder) {
      event.preventDefault();
      return;
    }
    setDraggedModId(modId);
    setDragTargetModId(modId);
    event.dataTransfer.effectAllowed = 'move';
    event.dataTransfer.setData('text/plain', modId);
  }

  function handleDragOver(event, modId) {
    if (!canReorder || !draggedModId || draggedModId === modId) {
      return;
    }
    event.preventDefault();
    event.dataTransfer.dropEffect = 'move';
    setDragTargetModId(modId);
  }

  async function handleDrop(event, targetModId) {
    if (!canReorder || !draggedModId || draggedModId === targetModId) {
      setDraggedModId('');
      setDragTargetModId('');
      return;
    }

    event.preventDefault();

    const fromIndex = mods.findIndex((mod) => mod.id === draggedModId);
    const toIndex = mods.findIndex((mod) => mod.id === targetModId);
    if (fromIndex === -1 || toIndex === -1 || fromIndex === toIndex) {
      setDraggedModId('');
      setDragTargetModId('');
      return;
    }

    const reorderedMods = [...mods];
    const [movedMod] = reorderedMods.splice(fromIndex, 1);
    reorderedMods.splice(toIndex, 0, movedMod);

    setDraggedModId('');
    setDragTargetModId('');
    await onReorder?.(reorderedMods.map((mod) => mod.id));
  }

  function handleDragEnd() {
    setDraggedModId('');
    setDragTargetModId('');
  }

  return (
    <section className="mods-table-section">
      <div className="mods-table-section__header">
        <div>
          <h3>{title}</h3>
          <p>{description}</p>
        </div>
        <span className="mods-table-section__count">{mods.length}</span>
      </div>

      <div className="table-wrap">
        <table className="data-table mods-table">
          <thead>
            <tr>
              <th>Mod</th>
              <th>Kind</th>
              <th>State</th>
              <th>Tracked files</th>
              <th>Primary path</th>
              <th>Action</th>
            </tr>
          </thead>
          <tbody>
            {mods.map((mod) => {
              const isBusy = togglingModId === mod.id;
              const actionLabel = isBusy
                ? 'Saving...'
                : mod.enabled === true
                  ? 'Disable'
                  : 'Enable';

              return (
                <tr
                  key={mod.id}
                  className={[
                    canReorder ? 'mods-table__row--draggable' : '',
                    draggedModId === mod.id ? 'mods-table__row--dragging' : '',
                    dragTargetModId === mod.id && draggedModId !== mod.id ? 'mods-table__row--drag-target' : '',
                  ]
                    .filter(Boolean)
                    .join(' ')}
                  draggable={canReorder && !isBusy}
                  onDragStart={(event) => handleDragStart(event, mod.id)}
                  onDragOver={(event) => handleDragOver(event, mod.id)}
                  onDrop={(event) => void handleDrop(event, mod.id)}
                  onDragEnd={handleDragEnd}
                >
                  <td>
                    <div className="mods-row__identity">
                      {draggable ? (
                        <span
                          className="mods-row__drag-handle"
                          title={reordering ? 'Saving load order...' : 'Drag to reorder'}
                        >
                          <SymbolIcon name="grip-vertical" />
                        </span>
                      ) : null}
                      <div className="mods-row__main">
                        <strong>{mod.name}</strong>
                        {mod.pluginName ? <code>{mod.pluginName}</code> : null}
                        {mod.notes?.length ? <p className="mods-row__note">{mod.notes[0]}</p> : null}
                        {mod.issues?.length ? <p className="mods-row__issue">{mod.issues[0].message}</p> : null}
                      </div>
                    </div>
                  </td>
                  <td>
                    <span className="pill pill--neutral">{modKindLabel(mod.kind)}</span>
                  </td>
                  <td>
                    <div className="mods-row__state">
                      <span className={`pill pill--${modStateTone(mod.state)}`}>
                        {humanizeLabel(mod.state)}
                      </span>
                      {mod.attention ? <span className="pill pill--bad">Attention</span> : null}
                    </div>
                  </td>
                  <td>{mod.totalFiles ?? 0}</td>
                  <td>
                    <div className="mods-row__path">
                      <code>{mod.primaryPath}</code>
                      {mod.relatedPaths?.length > 1 ? (
                        <span>+{mod.relatedPaths.length - 1} more tracked path(s)</span>
                      ) : null}
                    </div>
                  </td>
                  <td>
                    <div className="mods-row__actions">
                      <button
                        type="button"
                        className={mod.enabled === true ? 'ghost-button' : 'primary-button'}
                        disabled={isBusy || reordering || !mod.toggleable}
                        onClick={() => void onToggle(mod, mod.enabled !== true)}
                        title={mod.toggleable ? actionLabel : mod.toggleReason}
                      >
                        {mod.toggleable ? actionLabel : 'Unavailable'}
                      </button>
                      {!mod.toggleable && mod.toggleReason ? (
                        <span className="mods-row__action-note">{mod.toggleReason}</span>
                      ) : null}
                    </div>
                  </td>
                </tr>
              );
            })}
          </tbody>
        </table>
      </div>
    </section>
  );
}

function ModsInventoryTable({ mods, togglingModId, reordering, onReorder, onToggle }) {
  if (mods.length === 0) {
    return (
      <EmptyState
        title="No mods matched this view"
        description="Try clearing the search query or choosing a different filter."
      />
    );
  }

  const pluginBackedMods = mods.filter(
    (mod) => mod.kind === 'plugin_backed' || mod.kind === 'missing_plugin',
  );
  const inventoryOnlyMods = mods.filter((mod) => mod.state === 'inventory_only');

  return (
    <div className="stack">
      <ModsInventoryTableSection
        title="Plugin-Backed"
        description="Entries controlled through Plugins.txt, including plugin-backed mods and any plugin references that need attention."
        mods={pluginBackedMods}
        togglingModId={togglingModId}
        reordering={reordering}
        draggable
        onReorder={onReorder}
        onToggle={onToggle}
      />

      <ModsInventoryTableSection
        title="Inventory Only"
        description="Loose folder groups, SKSE plugins, and Skyrim Platform scripts that are currently tracked for visibility only."
        mods={inventoryOnlyMods}
        togglingModId={togglingModId}
        reordering={reordering}
        onToggle={onToggle}
      />
    </div>
  );
}

function TabButton({ active, icon, label, onClick }) {
  return (
    <button
      className={`tab-button ${active ? 'tab-button--active' : ''}`}
      onClick={onClick}
      type="button"
    >
      <span className="tab-button__icon">
        <SymbolIcon name={icon} />
      </span>
      <span>{label}</span>
    </button>
  );
}

function Notice({ notice, onDismiss }) {
  if (!notice) {
    return null;
  }

  return (
    <div className={`notice notice--${notice.type}`}>
      <div>
        <strong>{notice.title}</strong>
        <p>{notice.message}</p>
      </div>
      <button type="button" className="ghost-button" onClick={onDismiss}>
        Dismiss
      </button>
    </div>
  );
}

function EmptyState({ title, description }) {
  return (
    <div className="empty-state">
      <h3>{title}</h3>
      <p>{description}</p>
    </div>
  );
}

function formatPath(value) {
  return value && String(value).trim() ? value : 'Not configured';
}

function buildSyncResultSummary(result) {
  const touched = (result?.results ?? []).filter(
    (item) => item.result?.synced || item.result?.copiedFiles > 0,
  ).length;
  const skipped = (result?.results ?? []).filter((item) => item.result?.skipped).length;
  const stale = (result?.stale ?? []).reduce(
    (count, item) => count + (item.actions?.length ?? 0),
    0,
  );

  return {
    touched,
    skipped,
    stale,
  };
}

function BuildReadinessChecks({ requirements, compact = false }) {
  if (!requirements?.length) {
    return null;
  }

  const visibleRequirements = compact ? requirements.slice(0, 2) : requirements.slice(0, 3);
  const remainingRequirements = requirements.length - visibleRequirements.length;

  return (
    <div className={`build-checks ${compact ? 'build-checks--compact' : ''}`}>
      <span className="build-checks__label">Readiness checks</span>
      <div className="build-checks__list">
        {visibleRequirements.map((requirement) => (
          <span
            key={requirement.id}
            className={`build-check ${requirement.satisfied ? 'build-check--ok' : 'build-check--bad'}`}
          >
            <span className="build-check__dot" />
            {requirement.label}
          </span>
        ))}
        {remainingRequirements > 0 ? (
          <span className="build-check build-check--neutral">+{remainingRequirements} more</span>
        ) : null}
      </div>
    </div>
  );
}

function BuildRequirements({ requirements }) {
  if (!requirements?.length) {
    return null;
  }

  return (
    <div className="build-requirements">
      {requirements.map((requirement) => (
        <div
          key={requirement.id}
          className={`build-requirement ${requirement.satisfied ? 'build-requirement--ok' : 'build-requirement--bad'}`}
        >
          <span>{requirement.label}</span>
          <strong>{requirement.satisfied ? 'Ready' : 'Needs attention'}</strong>
        </div>
      ))}
    </div>
  );
}

function BuildProfileCard({
  profile,
  lastJob,
  featured = false,
  recommended = false,
  busy,
  hasLiveBuild,
  onRun,
  onOpenDetails,
  onOpenLogs,
}) {
  const visual = getProfileVisual(profile);
  const lastRunAt = lastJob?.startedAt || lastJob?.requestedAt;
  const lastRunMeta = !lastJob
    ? 'No completed runs yet'
    : lastJob.startedAt
      ? `${humanizeLabel(lastJob.status)} • ${formatDuration(lastRunAt, lastJob.endedAt)}`
      : humanizeLabel(lastJob.status);
  const runLabel = busy ? 'Starting...' : hasLiveBuild ? 'Queue build' : 'Run build';

  if (featured) {
    return (
      <article
        className="build-profile-card build-profile-card--featured"
        style={{
          '--build-accent': visual.accent,
          '--build-accent-soft': visual.soft,
          '--build-accent-border': visual.border,
        }}
      >
        <div className="build-profile-card__identity">
          <div className="build-profile-card__icon-shell">
            <SymbolIcon name={visual.icon} className="build-profile-card__icon" />
          </div>
          <div className="build-profile-card__content">
            <div className="build-profile-card__title-row">
              <div className="build-profile-card__copy">
                <div className="build-profile-card__title-line">
                  <h3>{profile.label}</h3>
                  <div className="build-profile-card__badges">
                    {recommended ? <span className="pill pill--info">Recommended</span> : null}
                    {profile.destructive ? <span className="pill pill--warn">Destructive</span> : null}
                    <span className={`pill pill--${profile.enabled ? 'ok' : 'warn'}`}>
                      {profile.enabled ? 'Ready' : 'Blocked'}
                    </span>
                  </div>
                </div>
                <p>{profile.description}</p>
              </div>
            </div>

            <div className="build-profile-card__meta">
              <span>
                Profile ID <code>{profile.id}</code>
              </span>
              <span>
                Mode <strong>{profile.executionMode}</strong>
              </span>
              <span>
                Outputs <strong>{profile.expectedOutputs?.length ?? 0}</strong>
              </span>
            </div>

            {profile.dependencies?.length ? (
              <div className="build-profile-card__dependencies">
                <span>Dependencies:</span>
                <strong>{profile.dependencies.join(', ')}</strong>
              </div>
            ) : null}

            <BuildReadinessChecks requirements={profile.requirements} />

            {!profile.enabled ? (
              <p className="build-profile-card__blocked">{profile.disabledReason}</p>
            ) : null}

            {profile.destructive ? (
              <div className="build-profile-card__warning">
                <strong>Destructive action</strong>
                <p>{profile.confirmMessage || 'This profile deletes the current build directory.'}</p>
              </div>
            ) : null}
          </div>
        </div>

        <div className="build-profile-card__side">
          <div className="build-profile-card__last-run">
            <span className="build-profile-card__eyebrow">Last run</span>
            <strong>{lastRunAt ? formatCompactDateTime(lastRunAt) : 'Never run'}</strong>
            <span>{lastRunMeta}</span>
          </div>

          <button
            type="button"
            className="primary-button build-run-button"
            disabled={busy || !profile.enabled}
            onClick={() => void onRun(profile)}
          >
            <SymbolIcon name="play" className="button-icon" />
            <span>{runLabel}</span>
          </button>

          <div className="build-profile-card__secondary-actions">
            <button
              type="button"
              className="ghost-button build-secondary-button"
              disabled={!lastJob}
              onClick={onOpenLogs}
            >
              <SymbolIcon name="terminal" className="button-icon" />
              <span>View logs</span>
            </button>
            <button
              type="button"
              className="ghost-button build-secondary-button"
              onClick={onOpenDetails}
            >
              <SymbolIcon name="info" className="button-icon" />
              <span>Details</span>
            </button>
          </div>
        </div>
      </article>
    );
  }

  return (
    <article
      className="build-profile-card build-profile-card--compact"
      style={{
        '--build-accent': visual.accent,
        '--build-accent-soft': visual.soft,
        '--build-accent-border': visual.border,
      }}
    >
      <div className="build-profile-card__compact-header">
        <div className="build-profile-card__icon-shell build-profile-card__icon-shell--compact">
          <SymbolIcon name={visual.icon} className="build-profile-card__icon" />
        </div>
        <div className="build-profile-card__compact-copy">
          <div className="build-profile-card__compact-title">
            <h3>{profile.label}</h3>
            <span className={`pill pill--${profile.enabled ? 'ok' : 'warn'}`}>
              {profile.enabled ? 'Ready' : 'Blocked'}
            </span>
          </div>
          <p>{profile.description}</p>
        </div>
      </div>

      <div className="build-profile-card__meta build-profile-card__meta--compact">
        <span>
          Mode <strong>{profile.executionMode}</strong>
        </span>
        <span>
          Outputs <strong>{profile.expectedOutputs?.length ?? 0}</strong>
        </span>
      </div>

      <BuildReadinessChecks requirements={profile.requirements} compact />

      {!profile.enabled ? (
        <p className="build-profile-card__blocked">{profile.disabledReason}</p>
      ) : null}

      <div className="build-profile-card__footer">
        <span className="build-profile-card__footer-copy">
          {lastRunAt ? `${formatStartedLabel(lastRunAt)} • ${humanizeLabel(lastJob?.status)}` : 'No recent runs'}
        </span>
        <div className="build-profile-card__footer-actions">
          <button
            type="button"
            className="icon-button icon-button--accent"
            disabled={busy || !profile.enabled}
            onClick={() => void onRun(profile)}
            aria-label={`${runLabel}: ${profile.label}`}
            title={runLabel}
          >
            <SymbolIcon name="play" />
          </button>
          <button
            type="button"
            className="icon-button"
            onClick={onOpenDetails}
            aria-label={`Open details for ${profile.label}`}
            title="Open details"
          >
            <SymbolIcon name="info" />
          </button>
        </div>
      </div>
    </article>
  );
}

function BuildProfileInfoModal({
  profile,
  busy,
  hasLiveBuild,
  updatingOptionKey,
  onClose,
  onRun,
  onSetCmakeOption,
}) {
  const closeButtonRef = useRef(null);
  const visual = getProfileVisual(profile);
  const runLabel = busy ? 'Starting...' : hasLiveBuild ? 'Queue build' : 'Run build';
  const dependencies = profile.dependencies?.length ? profile.dependencies.join(', ') : 'None';
  const outputs = profile.expectedOutputs ?? [];
  const cmakeOptions = profile.cmakeOptions ?? [];
  const metadataRows = [
    { icon: 'tag', label: 'Profile ID', value: profile.id },
    { icon: 'settings', label: 'Mode', value: humanizeLabel(profile.executionMode) },
    { icon: 'link', label: 'Dependencies', value: dependencies },
    { icon: 'document', label: 'Expected outputs', value: String(outputs.length) },
  ];

  useEffect(() => {
    const previousOverflow = document.body.style.overflow;
    const handleKeyDown = (event) => {
      if (event.key === 'Escape') {
        onClose();
      }
    };

    document.body.style.overflow = 'hidden';
    document.addEventListener('keydown', handleKeyDown);
    window.requestAnimationFrame(() => {
      closeButtonRef.current?.focus();
    });

    return () => {
      document.body.style.overflow = previousOverflow;
      document.removeEventListener('keydown', handleKeyDown);
    };
  }, [onClose]);

  async function handleRunClick() {
    const didStart = await onRun(profile);
    if (didStart) {
      onClose();
    }
  }

  async function handleToggleCmakeOption(option) {
    await onSetCmakeOption(profile, option.key, !option.value);
  }

  return (
    <div className="build-modal-backdrop" onClick={onClose}>
      <section
        aria-labelledby={`build-profile-modal-title-${profile.id}`}
        aria-modal="true"
        className="build-modal"
        onClick={(event) => event.stopPropagation()}
        role="dialog"
        style={{
          '--build-accent': visual.accent,
          '--build-accent-soft': visual.soft,
          '--build-accent-border': visual.border,
        }}
      >
        <div className="build-modal__header">
          <div className="build-modal__hero">
            <div className="build-modal__icon-shell">
              <SymbolIcon name={visual.icon} className="build-modal__hero-icon" />
            </div>
            <div className="build-modal__copy">
              <div className="build-modal__title-row">
                <div>
                  <h2 id={`build-profile-modal-title-${profile.id}`}>{profile.label}</h2>
                  <div className="build-modal__status-row">
                    <span className={`pill pill--${profile.enabled ? 'ok' : 'warn'}`}>
                      {profile.enabled ? 'Ready' : 'Blocked'}
                    </span>
                    {profile.destructive ? <span className="pill pill--warn">Destructive</span> : null}
                  </div>
                </div>
                <button
                  ref={closeButtonRef}
                  type="button"
                  className="build-modal__close-button"
                  onClick={onClose}
                  aria-label={`Close details for ${profile.label}`}
                >
                  <SymbolIcon name="x" />
                </button>
              </div>
              <p>{profile.description}</p>
            </div>
          </div>
        </div>

        <div className="build-modal__body">
          <div className="build-modal__meta-card">
            {metadataRows.map((row) => (
              <div className="build-modal__meta-row" key={row.label}>
                <div className="build-modal__meta-label">
                  <span className="build-modal__meta-icon">
                    <SymbolIcon name={row.icon} />
                  </span>
                  <span>{row.label}</span>
                </div>
                <strong className="build-modal__meta-value">{row.value}</strong>
              </div>
            ))}
          </div>

          <section className="build-modal__section">
            <h3>Readiness checks</h3>
            <div className="build-modal__list">
              {profile.requirements?.length ? (
                profile.requirements.map((requirement) => (
                  <div className="build-modal__list-item" key={requirement.id}>
                    <div className="build-modal__item-main">
                      <span
                        className={`build-modal__item-icon ${requirement.satisfied ? 'build-modal__item-icon--ok' : 'build-modal__item-icon--warn'}`}
                      >
                        <SymbolIcon name="check-circle" />
                      </span>
                      <span>{requirement.label}</span>
                    </div>
                    <span className={`pill pill--${requirement.satisfied ? 'ok' : 'warn'}`}>
                      {requirement.satisfied ? 'Ready' : 'Needs attention'}
                    </span>
                  </div>
                ))
              ) : (
                <div className="build-modal__empty">No readiness checks configured.</div>
              )}
            </div>
          </section>

          {cmakeOptions.length ? (
            <section className="build-modal__section">
              <div className="build-modal__section-copy">
                <h3>CMake options</h3>
                <p>Toggle the relevant cache flags here and the assistant will re-run CMake configure.</p>
              </div>
              <div className="build-modal__list">
                {cmakeOptions.map((option) => {
                  const isUpdating = updatingOptionKey === option.key;
                  const supported = option.supported !== false;
                  const actionLabel = isUpdating
                    ? 'Applying...'
                    : !supported
                      ? 'Unavailable'
                      : option.value
                      ? 'Turn off'
                      : 'Turn on';

                  return (
                    <div className="build-modal__list-item build-modal__list-item--option" key={option.key}>
                      <div className="build-modal__item-main build-modal__item-main--option">
                        <span className="build-modal__item-icon build-modal__item-icon--muted">
                          <SymbolIcon name="sliders" />
                        </span>
                        <span className="build-modal__option-copy">
                          <strong>{option.label}</strong>
                          <code>{option.key}</code>
                          <span>{option.description}</span>
                        </span>
                      </div>
                      <div className="build-modal__option-actions">
                        <span
                          className={`pill pill--${!supported ? 'neutral' : option.value ? 'ok' : 'warn'}`}
                        >
                          {!supported ? 'Unsupported' : option.value ? 'On' : 'Off'}
                        </span>
                        <button
                          type="button"
                          className="ghost-button ghost-button--compact"
                          disabled={isUpdating || !option.toggleable}
                          onClick={() => void handleToggleCmakeOption(option)}
                          title={option.toggleable ? actionLabel : option.disabledReason}
                        >
                          {actionLabel}
                        </button>
                      </div>
                      {!option.toggleable && option.disabledReason ? (
                        <span className="build-modal__option-note">{option.disabledReason}</span>
                      ) : (
                        <span className="build-modal__option-note">
                          Current value from{' '}
                          {option.source === 'cache'
                            ? 'CMakeCache.txt'
                            : option.source === 'stale-cache'
                              ? 'a stale CMakeCache.txt entry'
                              : 'CMake defaults'}
                          .
                        </span>
                      )}
                    </div>
                  );
                })}
              </div>
            </section>
          ) : null}

          <section className="build-modal__section">
            <h3>Outputs</h3>
            <div className="build-modal__list">
              {outputs.length ? (
                outputs.map((output) => {
                  const primaryText = output.path || output.label;
                  const secondaryText =
                    output.path && output.label && output.label !== output.path ? output.label : '';
                  return (
                    <div className="build-modal__list-item build-modal__list-item--output" key={output.path || output.label}>
                      <div className="build-modal__item-main">
                        <span className="build-modal__item-icon build-modal__item-icon--muted">
                          <SymbolIcon name="document" />
                        </span>
                        <span className="build-modal__output-copy">
                          <span>{primaryText}</span>
                          {secondaryText ? <span>{secondaryText}</span> : null}
                        </span>
                      </div>
                    </div>
                  );
                })
              ) : (
                <div className="build-modal__empty">No expected outputs configured.</div>
              )}
            </div>
          </section>

          {profile.destructive ? (
            <div className="build-modal__warning">
              <strong>Destructive action</strong>
              <p>{profile.confirmMessage || 'This profile deletes the configured build directory.'}</p>
            </div>
          ) : null}
        </div>

        <div className="build-modal__footer">
          <button
            type="button"
            className="primary-button build-modal__run-button"
            disabled={busy || !profile.enabled}
            onClick={() => void handleRunClick()}
          >
            <SymbolIcon name="play" className="button-icon" />
            <span>{runLabel}</span>
          </button>
          <button type="button" className="ghost-button build-modal__close-action" onClick={onClose}>
            Close
          </button>
        </div>
      </section>
    </div>
  );
}

function BuildJobDetails({ title, job, onCancel, cancelling, compact = false }) {
  return (
    <section className={`panel ${compact ? 'build-sidebar-panel build-sidebar-panel--detail' : ''}`}>
      <div className="panel__header panel__header--wrap">
        <div>
          <h2>{title}</h2>
          <p>Step-by-step state, timings, and the command trail for the selected build.</p>
        </div>
        {job && ['queued', 'running'].includes(job.status) ? (
          <button
            type="button"
            className="ghost-button"
            disabled={cancelling}
            onClick={() => void onCancel(job.id)}
          >
            {cancelling ? 'Cancelling...' : 'Cancel build'}
          </button>
        ) : null}
      </div>

      {!job ? (
        <EmptyState
          title="No build selected"
          description="Run a build or select one from recent history to inspect it."
        />
      ) : (
        <div className="stack">
          <div className={`build-job-meta ${compact ? 'build-job-meta--compact' : ''}`}>
            <div className="key-value">
              <span>Status</span>
              <span className={`pill pill--${buildTone(job.status)}`}>{job.status}</span>
            </div>
            <div className="key-value">
              <span>Started</span>
              <strong>{formatDateTime(job.startedAt || job.requestedAt)}</strong>
            </div>
            <div className="key-value">
              <span>Duration</span>
              <strong>{formatDuration(job.startedAt || job.requestedAt, job.endedAt)}</strong>
            </div>
            <div className="key-value">
              <span>Requested by</span>
              <strong>{job.requestedBy}</strong>
            </div>
          </div>

          <div className={`build-step-list ${compact ? 'build-step-list--compact' : ''}`}>
            {job.steps.map((step) => (
              <article className="build-step" key={`${job.id}-${step.id}`}>
                <div className="build-step__header">
                  <strong>{step.label}</strong>
                  <span className={`pill pill--${buildTone(step.status)}`}>{step.status}</span>
                </div>
                <code>{step.command}</code>
                <div className="build-step__meta">
                  <span>{formatDateTime(step.startedAt)}</span>
                  <span>{step.cwd}</span>
                </div>
              </article>
            ))}
          </div>

          {job.missingOutputs?.length ? (
            <div className="build-missing-output-list">
              <strong>Expected outputs were missing after the build:</strong>
              {job.missingOutputs.map((output) => (
                <code key={output.path}>{output.label}: {output.path}</code>
              ))}
            </div>
          ) : null}
        </div>
      )}
    </section>
  );
}

function BuildQueuePanel({ jobs, selectedJobId, onSelect }) {
  return (
    <section className="panel build-sidebar-panel">
      <div className="panel__header build-sidebar-panel__header">
        <div className="build-sidebar-panel__title-row">
          <h2>Queue</h2>
          <span className="build-count-badge">{jobs.length}</span>
        </div>
      </div>

      {jobs.length === 0 ? (
        <div className="build-queue-empty">
          <div className="build-queue-empty__icon">
            <SymbolIcon name="queue" />
          </div>
          <strong>The queue is empty</strong>
          <p>Launch a profile to add jobs here.</p>
        </div>
      ) : (
        <div className="build-queue-list">
          {jobs.map((job) => (
            <button
              key={job.id}
              type="button"
              className={`build-queue-item ${selectedJobId === job.id ? 'build-queue-item--active' : ''}`}
              onClick={() => onSelect(job.id)}
            >
              <div>
                <strong>{job.profileLabel}</strong>
                <p>Queue position {job.queuePosition}</p>
              </div>
              <span className={`pill pill--${buildTone(job.status)}`}>{humanizeLabel(job.status)}</span>
            </button>
          ))}
        </div>
      )}
    </section>
  );
}

function BuildHistory({ jobs, selectedJobId, onSelect, onOpenLogs, onRunProfile }) {
  const [expanded, setExpanded] = useState(false);
  const visibleJobs = expanded ? jobs : jobs.slice(0, 5);

  return (
    <section className="panel build-sidebar-panel">
      <div className="panel__header build-sidebar-panel__header build-sidebar-panel__header--spread">
        <div className="build-sidebar-panel__title-row">
          <h2>Recent jobs</h2>
        </div>
        {jobs.length > 5 ? (
          <button
            type="button"
            className="ghost-button ghost-button--compact"
            onClick={() => setExpanded((current) => !current)}
          >
            {expanded ? 'Collapse' : 'View all'}
          </button>
        ) : null}
      </div>

      {jobs.length === 0 ? (
        <EmptyState
          title="No jobs yet"
          description="Your build history will appear here after you run a profile."
        />
      ) : (
        <div className="build-history">
          <div className="build-history__head">
            <span>Job</span>
            <span>Status</span>
            <span>Started</span>
            <span>Duration</span>
            <span>Actions</span>
          </div>
          <div className="build-history__list">
            {visibleJobs.map((job) => (
              <article
                className={`build-history-row ${selectedJobId === job.id ? 'build-history-row--active' : ''}`}
                key={job.id}
              >
                <button
                  type="button"
                  className="build-history-row__select"
                  onClick={() => onSelect(job.id)}
                >
                  <div className="build-history-row__job">
                    <strong>{job.profileLabel}</strong>
                    <span>{job.profileId}</span>
                  </div>
                  <span className={`pill pill--${buildTone(job.status)}`}>{humanizeLabel(job.status)}</span>
                  <span>{formatStartedLabel(job.startedAt || job.requestedAt)}</span>
                  <span>{formatDuration(job.startedAt || job.requestedAt, job.endedAt)}</span>
                </button>
                <div className="build-history-row__actions">
                  <button
                    type="button"
                    className="icon-button"
                    onClick={() => onRunProfile(job)}
                    aria-label={`Run ${job.profileLabel} again`}
                    title="Run again"
                  >
                    <SymbolIcon name="refresh" />
                  </button>
                  <button
                    type="button"
                    className="icon-button"
                    onClick={() => onOpenLogs(job.id)}
                    aria-label={`Open logs for ${job.profileLabel}`}
                    title="Open logs"
                  >
                    <SymbolIcon name="terminal" />
                  </button>
                </div>
              </article>
            ))}
          </div>

          {jobs.length > 5 ? (
            <button
              type="button"
              className="build-history__more"
              onClick={() => setExpanded((current) => !current)}
            >
              {expanded ? 'Show less' : 'Show more'}
            </button>
          ) : null}
        </div>
      )}
    </section>
  );
}

function BuildLogPanel({ job, compact = false }) {
  const logRef = useRef(null);
  const [stickToBottom, setStickToBottom] = useState(true);

  useEffect(() => {
    setStickToBottom(true);
  }, [job?.id]);

  useEffect(() => {
    if (!job || !stickToBottom) {
      return;
    }

    const element = logRef.current;
    if (!element) {
      return;
    }

    window.requestAnimationFrame(() => {
      element.scrollTop = element.scrollHeight;
    });
  }, [job, job?.logs?.length, stickToBottom]);

  function handleLogScroll() {
    const element = logRef.current;
    if (!element) {
      return;
    }

    const threshold = 24;
    const distanceFromBottom = element.scrollHeight - element.scrollTop - element.clientHeight;
    const nextStickToBottom = distanceFromBottom <= threshold;
    setStickToBottom((current) => (current === nextStickToBottom ? current : nextStickToBottom));
  }

  return (
    <section className={`panel ${compact ? 'build-sidebar-panel build-sidebar-panel--detail' : ''}`}>
      <div className="panel__header">
        <div>
          <h2>Logs</h2>
          <p>Polling-based live log view for the active or selected job.</p>
        </div>
      </div>

      {!job ? (
        <EmptyState
          title="No logs available"
          description="Start a build or select a historical job to inspect its captured output."
        />
      ) : job.logs?.length ? (
        <pre
          ref={logRef}
          className={`build-log-view ${compact ? 'build-log-view--compact' : ''}`}
          onScroll={handleLogScroll}
        >
          {job.logs.map((entry) => `[${entry.stream}] ${entry.message}`).join('\n')}
        </pre>
      ) : (
        <EmptyState
          title="No logs captured yet"
          description="This job has not produced any stdout or stderr output so far."
        />
      )}
    </section>
  );
}

export function App() {
  const [activeTab, setActiveTab] = useState('overview');
  const [meta, setMeta] = useState(null);
  const [statusData, setStatusData] = useState(null);
  const [doctorData, setDoctorData] = useState(null);
  const [configData, setConfigData] = useState(null);
  const [formValues, setFormValues] = useState({});
  const [syncFilter, setSyncFilter] = useState('all');
  const [modSearch, setModSearch] = useState('');
  const [modFilter, setModFilter] = useState('all');
  const [profileSearch, setProfileSearch] = useState('');
  const [buildCategoryFilter, setBuildCategoryFilter] = useState('all');
  const [buildFilterOpen, setBuildFilterOpen] = useState(false);
  const [loading, setLoading] = useState(true);
  const [refreshing, setRefreshing] = useState(false);
  const [syncing, setSyncing] = useState(false);
  const [saving, setSaving] = useState(false);
  const [saveErrors, setSaveErrors] = useState({});
  const [syncResult, setSyncResult] = useState(null);
  const [notice, setNotice] = useState(null);
  const [fatalError, setFatalError] = useState('');
  const [selectedBuildJobId, setSelectedBuildJobId] = useState(null);
  const [selectedProfileInfoId, setSelectedProfileInfoId] = useState('');
  const [startingProfileId, setStartingProfileId] = useState('');
  const [updatingBuildOptionKey, setUpdatingBuildOptionKey] = useState('');
  const [cancellingJobId, setCancellingJobId] = useState('');
  const [launchingRuntime, setLaunchingRuntime] = useState(false);
  const [openingRuntimeFolder, setOpeningRuntimeFolder] = useState(false);
  const [stoppingRuntime, setStoppingRuntime] = useState(false);

  const loadMeta = useCallback(async () => {
    const data = await getMeta();
    setMeta(data);
    return data;
  }, []);

  const loadStatus = useCallback(async () => {
    const data = await getStatus();
    setStatusData(data);
    return data;
  }, []);

  const loadDoctor = useCallback(async () => {
    const data = await getDoctor();
    setDoctorData(data);
    return data;
  }, []);

  const loadConfig = useCallback(async () => {
    const data = await getConfig();
    setConfigData(data);
    setFormValues(data.values ?? {});
    return data;
  }, []);

  const loadAll = useCallback(
    async ({ silent = false } = {}) => {
      if (silent) {
        setRefreshing(true);
      } else {
        setLoading(true);
      }

      try {
        setFatalError('');
        await Promise.all([loadMeta(), loadStatus(), loadDoctor(), loadConfig()]);
      } catch (error) {
        setFatalError(error.message || 'Failed to load dashboard data.');
      } finally {
        setLoading(false);
        setRefreshing(false);
      }
    },
    [loadConfig, loadDoctor, loadMeta, loadStatus],
  );

  useEffect(() => {
    void loadAll();
  }, [loadAll]);

  const handleBuildSettled = useCallback(
    (job) => {
      if (job.status === 'succeeded') {
        setNotice({
          type: 'success',
          title: 'Build completed',
          message: `${job.profileLabel} finished successfully.`,
        });
      } else if (job.status === 'cancelled') {
        setNotice({
          type: 'error',
          title: 'Build cancelled',
          message: `${job.profileLabel} was cancelled before it finished.`,
        });
      } else {
        const detail = job.missingOutputs?.length
          ? ` Expected output was missing: ${job.missingOutputs[0].label}.`
          : '';
        setNotice({
          type: 'error',
          title: 'Build failed',
          message: `${job.profileLabel} did not complete successfully.${detail}`,
        });
      }
      void loadAll({ silent: true });
    },
    [loadAll],
  );

  const {
    snapshot: modsSnapshot,
    mods,
    summary: modsSummary,
    loading: modsLoading,
    refreshing: modsRefreshing,
    reordering: modsReordering,
    togglingModId,
    error: modsError,
    refreshMods,
    reorderMods,
    toggleMod,
  } = useModsMonitor();
  const {
    profiles,
    snapshot: buildSnapshot,
    activeJob,
    queuedJobs,
    recentJobs,
    loading: buildLoading,
    refreshing: buildRefreshing,
    error: buildError,
    refreshBuilds,
    runProfile,
    cancelJob,
    updateCmakeOption,
  } = useBuildMonitor({
    onSettled: handleBuildSettled,
  });
  const {
    runtime,
    loading: runtimeLoading,
    refreshing: runtimeRefreshing,
    error: runtimeError,
    refreshRuntime,
    requestLaunch,
    requestOpenFolder,
    requestStop,
  } = useRuntimeMonitor();

  const rows = statusData?.rows ?? [];
  const doctorIssues = doctorData?.issues ?? [];
  const statusSummary = statusData?.summary ?? { total: 0, byStatus: {} };
  const syncSummary = useMemo(() => buildSyncResultSummary(syncResult), [syncResult]);
  const hasLiveBuild = Boolean(activeJob) || queuedJobs.length > 0;

  const filteredRows = useMemo(() => {
    if (syncFilter === 'all') {
      return rows;
    }
    if (syncFilter === 'attention') {
      return rows.filter((row) =>
        ['dirty', 'missing_source', 'skipped_no_skyrim'].includes(row.status),
      );
    }
    return rows.filter((row) => row.status === syncFilter);
  }, [rows, syncFilter]);

  const filteredMods = useMemo(() => {
    const query = modSearch.trim().toLowerCase();
    return mods.filter((mod) => {
      const matchesQuery =
        !query ||
        [
          mod.name,
          mod.kind,
          mod.pluginName,
          mod.primaryPath,
          ...(mod.relatedPaths ?? []),
          ...(mod.issues ?? []).map((issue) => issue.message),
        ]
          .filter(Boolean)
          .some((value) => String(value).toLowerCase().includes(query));

      if (!matchesQuery) {
        return false;
      }

      if (modFilter === 'all') {
        return true;
      }
      if (modFilter === 'attention') {
        return mod.attention === true;
      }
      if (modFilter === 'toggleable') {
        return mod.toggleable === true;
      }
      return mod.state === modFilter;
    });
  }, [modFilter, modSearch, mods]);

  useEffect(() => {
    if (activeJob?.id) {
      setSelectedBuildJobId(activeJob.id);
      return;
    }

    const availableJobIds = new Set(buildSnapshot.jobs.map((job) => job.id));
    if (selectedBuildJobId && availableJobIds.has(selectedBuildJobId)) {
      return;
    }

    if (recentJobs[0]?.id) {
      setSelectedBuildJobId(recentJobs[0].id);
    }
  }, [activeJob?.id, buildSnapshot.jobs, recentJobs, selectedBuildJobId]);

  const selectedBuildJob = useMemo(() => {
    return (
      buildSnapshot.jobs.find((job) => job.id === selectedBuildJobId) ??
      activeJob ??
      recentJobs[0] ??
      null
    );
  }, [activeJob, buildSnapshot.jobs, recentJobs, selectedBuildJobId]);

  const buildJobs = buildSnapshot.jobs ?? [];
  const readyProfilesCount = useMemo(
    () => profiles.filter((profile) => profile.enabled).length,
    [profiles],
  );
  const lastJobByProfileId = useMemo(() => {
    const jobsByProfileId = new Map();
    for (const job of buildJobs) {
      if (!jobsByProfileId.has(job.profileId)) {
        jobsByProfileId.set(job.profileId, job);
      }
    }
    return jobsByProfileId;
  }, [buildJobs]);
  const buildCategories = useMemo(
    () => Array.from(new Set(profiles.map((profile) => profile.category).filter(Boolean))),
    [profiles],
  );
  const filteredProfiles = useMemo(() => {
    const query = profileSearch.trim().toLowerCase();
    return profiles.filter((profile) => {
      const matchesQuery =
        !query ||
        [profile.label, profile.id, profile.description, profile.category]
          .filter(Boolean)
          .some((value) => String(value).toLowerCase().includes(query));
      const matchesCategory =
        buildCategoryFilter === 'all' || profile.category === buildCategoryFilter;
      return matchesQuery && matchesCategory;
    });
  }, [buildCategoryFilter, profileSearch, profiles]);
  const featuredProfile = useMemo(() => {
    return (
      filteredProfiles.find((profile) => profile.id === 'cmake-default') ??
      filteredProfiles[0] ??
      null
    );
  }, [filteredProfiles]);
  const compactProfiles = useMemo(() => {
    return filteredProfiles.filter((profile) => profile.id !== featuredProfile?.id);
  }, [featuredProfile?.id, filteredProfiles]);
  const selectedProfileInfo = useMemo(() => {
    return profiles.find((profile) => profile.id === selectedProfileInfoId) ?? null;
  }, [profiles, selectedProfileInfoId]);
  const lastFinishedJob = useMemo(() => {
    return (
      buildJobs.find((job) => job.id === buildSnapshot.summary?.lastFinishedJobId) ??
      buildJobs.find((job) => ['succeeded', 'failed', 'cancelled'].includes(job.status)) ??
      null
    );
  }, [buildJobs, buildSnapshot.summary?.lastFinishedJobId]);
  const lastUpdatedAt = useMemo(() => {
    if (loading || modsLoading || buildLoading || runtimeLoading) {
      return null;
    }
    return Date.now();
  }, [
    loading,
    modsLoading,
    buildLoading,
    runtimeLoading,
    meta,
    statusData,
    doctorData,
    configData,
    modsSnapshot,
    buildSnapshot,
    profiles,
    runtime?.checkedAt,
  ]);
  const headerSubtitle = TAB_SUBTITLES[activeTab] ?? TAB_SUBTITLES.overview;

  useEffect(() => {
    if (activeTab !== 'builds' && selectedProfileInfoId) {
      setSelectedProfileInfoId('');
    }
  }, [activeTab, selectedProfileInfoId]);

  async function handleSync(dryRun) {
    setSyncing(true);
    try {
      const result = await runSyncRequest({ dryRun });
      setSyncResult(result);
      setNotice({
        type: 'success',
        title: dryRun ? 'Dry run complete' : 'Sync complete',
        message: dryRun
          ? 'The dashboard checked which files would change.'
          : 'The dashboard finished syncing tracked artifacts.',
      });
      await Promise.all([loadStatus(), loadDoctor()]);
    } catch (error) {
      setNotice({
        type: 'error',
        title: 'Sync failed',
        message: error.message || 'Could not complete sync.',
      });
    } finally {
      setSyncing(false);
    }
  }

  async function handleSaveSettings() {
    setSaving(true);
    setSaveErrors({});

    try {
      const data = await saveConfig(formValues);
      setConfigData(data);
      setFormValues(data.values ?? {});
      setNotice({
        type: 'success',
        title: 'Settings saved',
        message: `${data.configPath} was updated successfully.`,
      });
      await Promise.all([
        loadMeta(),
        loadStatus(),
        loadDoctor(),
        refreshMods({ silent: true }),
        refreshBuilds({ silent: true }),
        refreshRuntime({ silent: true }),
      ]);
    } catch (error) {
      const nextErrors = Object.fromEntries(
        (error.details ?? []).map((detail) => [detail.field, detail.message]),
      );
      setSaveErrors(nextErrors);
      setNotice({
        type: 'error',
        title: 'Could not save settings',
        message: error.message || 'Please fix the highlighted fields and try again.',
      });
    } finally {
      setSaving(false);
    }
  }

  async function handleRunBuild(profile) {
    const profileId = profile.id;
    if (profile.destructive) {
      const confirmed = window.confirm(
        profile.confirmMessage || 'This profile deletes the configured build directory. Continue?',
      );
      if (!confirmed) {
        return false;
      }
    }

    setStartingProfileId(profileId);
    try {
      const response = await runProfile(profileId, {
        confirmDestructive: profile.destructive === true,
      });
      setActiveTab('builds');
      setSelectedBuildJobId(response.job?.id ?? null);
      setNotice({
        type: 'success',
        title: response.job?.status === 'queued' ? 'Build queued' : 'Build started',
        message:
          response.job?.status === 'queued'
            ? `${response.job.profileLabel} was queued behind the active build.`
            : `${response.job?.profileLabel ?? profileId} is now running.`,
      });
      return true;
    } catch (error) {
      setNotice({
        type: 'error',
        title: 'Could not start build',
        message: error.message || 'The selected build profile could not be started.',
      });
      return false;
    } finally {
      setStartingProfileId('');
    }
  }

  async function handleCancelBuild(jobId = null) {
    setCancellingJobId(jobId ?? activeJob?.id ?? '');
    try {
      const response = await cancelJob(jobId);
      setNotice({
        type: 'success',
        title: 'Cancellation requested',
        message:
          response.source === 'queue'
            ? 'The queued build was removed before it started.'
            : 'The active build is being stopped.',
      });
    } catch (error) {
      setNotice({
        type: 'error',
        title: 'Could not cancel build',
        message: error.message || 'The selected build job could not be cancelled.',
      });
    } finally {
      setCancellingJobId('');
    }
  }

  async function handleSetBuildOption(profile, optionKey, enabled) {
    setUpdatingBuildOptionKey(optionKey);
    try {
      await updateCmakeOption(optionKey, enabled);
      setNotice({
        type: 'success',
        title: 'CMake option updated',
        message: `${optionKey} is now ${enabled ? 'ON' : 'OFF'} for ${profile.label}.`,
      });
      return true;
    } catch (error) {
      setNotice({
        type: 'error',
        title: 'Could not update CMake option',
        message: error.message || `Failed to update ${optionKey}.`,
      });
      return false;
    } finally {
      setUpdatingBuildOptionKey('');
    }
  }

  async function handleLaunchRuntime() {
    setLaunchingRuntime(true);
    try {
      const result = await requestLaunch();
      setNotice({
        type: 'success',
        title: result.action === 'already_running' ? 'Skyrim already running' : 'Launch requested',
        message:
          result.action === 'already_running'
            ? 'A Skyrim process was already detected.'
            : result.status?.running
              ? 'Skyrim started successfully.'
              : 'SKSE launch was requested. The runtime panel will update as the game starts.',
      });
    } catch (error) {
      setNotice({
        type: 'error',
        title: 'Could not launch Skyrim',
        message: error.message || 'Runtime launch failed.',
      });
    } finally {
      setLaunchingRuntime(false);
    }
  }

  async function handleStopRuntime() {
    setStoppingRuntime(true);
    try {
      const result = await requestStop();
      setNotice({
        type: 'success',
        title: result.action === 'not_running' ? 'Runtime already idle' : 'Force stop requested',
        message:
          result.action === 'not_running'
            ? 'No Skyrim process was running.'
            : `Requested shutdown for ${result.stoppedProcessCount} Skyrim process(s).`,
      });
    } catch (error) {
      setNotice({
        type: 'error',
        title: 'Could not stop Skyrim',
        message: error.message || 'Runtime stop failed.',
      });
    } finally {
      setStoppingRuntime(false);
    }
  }

  async function handleOpenRuntimeFolder() {
    setOpeningRuntimeFolder(true);
    try {
      const result = await requestOpenFolder();
      setNotice({
        type: 'success',
        title: 'Opened game folder',
        message: `${result.folderPath || runtime?.skyrimRoot || 'Skyrim folder'} was opened in Explorer.`,
      });
    } catch (error) {
      setNotice({
        type: 'error',
        title: 'Could not open game folder',
        message: error.message || 'Opening the configured Skyrim folder failed.',
      });
    } finally {
      setOpeningRuntimeFolder(false);
    }
  }

  async function handleToggleMod(mod, enabled) {
    try {
      const response = await toggleMod(mod.id, enabled);
      setNotice({
        type: 'success',
        title: enabled ? 'Mod enabled' : 'Mod disabled',
        message: `${response.mod?.pluginName ?? mod.pluginName ?? mod.name} was ${enabled ? 'enabled' : 'disabled'} through Plugins.txt.`,
      });
      await loadDoctor();
    } catch (error) {
      setNotice({
        type: 'error',
        title: 'Could not update mod state',
        message: error.message || 'The selected mod could not be toggled.',
      });
      await refreshMods({ silent: true });
      await loadDoctor();
    }
  }

  async function handleReorderMods(orderedModIds) {
    try {
      await reorderMods(orderedModIds);
      await loadDoctor();
    } catch (error) {
      setNotice({
        type: 'error',
        title: 'Could not update mod load order',
        message: error.message || 'The requested Plugins.txt reorder failed.',
      });
      await refreshMods({ silent: true });
      await loadDoctor();
    }
  }

  async function handleRefresh() {
    await Promise.all([
      loadAll({ silent: true }),
      refreshMods({ silent: true }),
      refreshBuilds({ silent: true }),
      refreshRuntime({ silent: true }),
    ]);
  }

  const handleSelectBuildJob = useCallback((jobId) => {
    setSelectedBuildJobId(jobId);
  }, []);

  function handleFieldChange(key, value) {
    setFormValues((current) => ({
      ...current,
      [key]: value,
    }));
  }

  function handleResetSettings() {
    setFormValues(configData?.values ?? {});
    setSaveErrors({});
  }

  const overviewCards = [
    {
      label: 'Repo root',
      value: meta?.repoRoot,
    },
    {
      label: 'Config file',
      value: meta?.configPath,
    },
    {
      label: 'Skyrim root',
      value: formatPath(meta?.ctx?.skyrimRoot),
    },
    {
      label: 'Build directory',
      value: formatPath(meta?.ctx?.buildDir),
    },
    {
      label: 'SKSE loader',
      value: formatPath(meta?.ctx?.skseLoaderPath),
    },
    {
      label: 'Plugins.txt',
      value: formatPath(meta?.ctx?.pluginsTxtPath),
    },
    {
      label: 'NirnLab output',
      value: formatPath(meta?.ctx?.nirnLabOutputDir),
    },
    {
      label: 'Dashboard port',
      value: meta?.dashboard?.port ? `127.0.0.1:${meta.dashboard.port}` : 'Unknown',
    },
  ];

  if (loading || modsLoading || buildLoading || runtimeLoading) {
    return (
      <div className="loading-screen">
        <div className="loading-screen__panel">
          <h1>SkyMP Build Assistant</h1>
          <p>Loading dashboard data...</p>
        </div>
      </div>
    );
  }

  return (
    <div className="app-shell">
      <header className="app-header">
        <div className="app-header__brand">
          <div className="app-header__icon">
            <SymbolIcon name="cube" />
          </div>
          <div>
            <p className="eyebrow">Local Dev Dashboard</p>
            <h1>SkyMP Build Assistant</h1>
            <p className="app-header__subtitle">{headerSubtitle}</p>
          </div>
        </div>
        <div className="header-actions">
          <div className="header-actions__buttons">
            <button
              type="button"
              className="ghost-button"
              disabled={refreshing || modsRefreshing || buildRefreshing}
              onClick={() => void handleRefresh()}
            >
              <SymbolIcon name="refresh" className="button-icon" />
              <span>{refreshing || modsRefreshing || buildRefreshing ? 'Refreshing...' : 'Refresh status'}</span>
            </button>
            <button
              type="button"
              className="primary-button"
              disabled={syncing}
              onClick={() => void handleSync(false)}
            >
              <SymbolIcon name="sync" className="button-icon" />
              <span>{syncing ? 'Syncing...' : 'Sync artifacts'}</span>
            </button>
          </div>
          <span className="header-actions__status">
            <span className="status-dot status-dot--ok" />
            {formatLastUpdatedLabel(lastUpdatedAt)}
          </span>
        </div>
      </header>

      <Notice notice={notice} onDismiss={() => setNotice(null)} />

      {fatalError ? (
        <div className="error-panel">
          <h2>Dashboard failed to load</h2>
          <p>{fatalError}</p>
          <button
            type="button"
            className="primary-button"
            onClick={() => void loadAll()}
          >
            Retry
          </button>
        </div>
      ) : (
        <>
          <nav className="tab-bar" aria-label="Dashboard navigation">
            {TABS.map((tab) => (
              <TabButton
                key={tab.id}
                active={activeTab === tab.id}
                icon={tab.icon}
                label={tab.label}
                onClick={() => setActiveTab(tab.id)}
              />
            ))}
          </nav>

          {activeTab === 'overview' ? (
            <section className="page-grid">
              <div className="page-column page-column--wide">
                <section className="panel hero-panel">
                  <div className="hero-panel__copy">
                    <h2>Project snapshot</h2>
                    <p>
                      See whether your local SkyMP setup is configured, whether artifacts
                      look healthy, and jump directly into fixing issues.
                    </p>
                  </div>
                  <div className="hero-panel__actions">
                    <button
                      type="button"
                      className="primary-button"
                      onClick={() => setActiveTab('builds')}
                    >
                      Open Builds
                    </button>
                    <button
                      type="button"
                      className="ghost-button"
                      onClick={() => setActiveTab('sync')}
                    >
                      Review Sync
                    </button>
                    <button
                      type="button"
                      className="ghost-button"
                      onClick={() => setActiveTab('mods')}
                    >
                      Open Mods
                    </button>
                  </div>
                </section>

                <SyncSummary summary={statusSummary} />
                <BuildSummary snapshot={buildSnapshot} profiles={profiles} />
                <ModsSummary snapshot={modsSnapshot} />
                <RuntimeSummary runtime={runtime} />
                <DoctorSummary issues={doctorIssues} />

                <section className="panel">
                  <div className="panel__header">
                    <div>
                      <h2>Environment</h2>
                      <p>The current dashboard context and resolved path configuration.</p>
                    </div>
                  </div>
                  <div className="info-grid">
                    {overviewCards.map((item) => (
                      <article className="info-card" key={item.label}>
                        <span className="info-card__label">{item.label}</span>
                        <code>{item.value}</code>
                      </article>
                    ))}
                  </div>
                </section>
              </div>

              <div className="page-column">
                <RuntimePanel
                  runtime={runtime}
                  error={runtimeError}
                  launching={launchingRuntime}
                  openingFolder={openingRuntimeFolder}
                  stopping={stoppingRuntime}
                  refreshing={runtimeRefreshing}
                  onLaunch={handleLaunchRuntime}
                  onOpenFolder={handleOpenRuntimeFolder}
                  onStop={handleStopRuntime}
                  onOpenSettings={() => setActiveTab('settings')}
                />

                <section className="panel">
                  <div className="panel__header">
                    <div>
                      <h2>Build activity</h2>
                      <p>Current control-plane view of the active queue and latest result.</p>
                    </div>
                    <button
                      type="button"
                      className="ghost-button"
                      onClick={() => setActiveTab('builds')}
                    >
                      Open Builds
                    </button>
                  </div>
                  {activeJob ? (
                    <div className="stack">
                      <div className="key-value">
                        <span>Active job</span>
                        <strong>{activeJob.profileLabel}</strong>
                      </div>
                      <div className="key-value">
                        <span>Status</span>
                        <span className={`pill pill--${buildTone(activeJob.status)}`}>
                          {activeJob.status}
                        </span>
                      </div>
                      <div className="key-value">
                        <span>Queued behind it</span>
                        <strong>{queuedJobs.length}</strong>
                      </div>
                    </div>
                  ) : (
                    <EmptyState
                      title="Builds are idle"
                      description="No build is currently running. Launch one from the Builds tab."
                    />
                  )}
                </section>

                <section className="panel">
                  <div className="panel__header">
                    <div>
                      <h2>Doctor highlights</h2>
                      <p>Most important environment issues right now.</p>
                    </div>
                    <button
                      type="button"
                      className="ghost-button"
                      onClick={() => setActiveTab('doctor')}
                    >
                      View all
                    </button>
                  </div>

                  {doctorIssues.length === 0 ? (
                    <EmptyState
                      title="No doctor issues"
                      description="Your current environment checks are clean."
                    />
                  ) : (
                    <div className="issue-list">
                      {doctorIssues.slice(0, 4).map((issue) => (
                        <article
                          key={`${issue.level}-${issue.code}`}
                          className={`issue-card issue-card--${issue.level}`}
                        >
                          <div className="issue-card__header">
                            <span className={`pill pill--${issue.level}`}>{issue.level}</span>
                            <strong>{issue.code}</strong>
                          </div>
                          <p>{issue.message}</p>
                        </article>
                      ))}
                    </div>
                  )}
                </section>

                <section className="panel">
                  <div className="panel__header">
                    <div>
                      <h2>Configuration</h2>
                      <p>
                        The dashboard now creates and updates your local YAML config file.
                      </p>
                    </div>
                  </div>
                  <div className="stack">
                    <div className="key-value">
                      <span>Config file exists</span>
                      <strong>{meta?.configExists ? 'Yes' : 'No'}</strong>
                    </div>
                    <div className="key-value">
                      <span>UI build</span>
                      <strong>{meta?.ui?.built ? 'Built' : 'Missing build output'}</strong>
                    </div>
                    <div className="key-value">
                      <span>Tracked artifacts</span>
                      <strong>{statusSummary.total}</strong>
                    </div>
                  </div>
                </section>
              </div>
            </section>
          ) : null}

          {activeTab === 'builds' ? (
            <section className="page-grid page-grid--builds">
              <div className="page-column page-column--wide">
                <div className="build-metrics-grid">
                  <BuildMetricCard
                    icon="check-circle"
                    label="Ready profiles"
                    value={readyProfilesCount}
                    secondaryValue={`/${profiles.length}`}
                    meta={
                      readyProfilesCount === profiles.length
                        ? 'All profiles ready'
                        : `${profiles.length - readyProfilesCount} need attention`
                    }
                    tone="ok"
                  />
                  <BuildMetricCard
                    icon="queue"
                    label="Queued jobs"
                    value={queuedJobs.length}
                    meta={queuedJobs.length ? `${queuedJobs[0].profileLabel} is first in line` : 'No jobs waiting'}
                    tone={queuedJobs.length ? 'warn' : 'neutral'}
                  />
                  <BuildMetricCard
                    icon="activity"
                    label="Active build"
                    value={activeJob?.profileLabel ?? 'None'}
                    meta={activeJob ? humanizeLabel(activeJob.status) : 'All clear'}
                    tone={activeJob ? buildTone(activeJob.status) : 'neutral'}
                  />
                  <BuildMetricCard
                    icon="clock"
                    label="Last job status"
                    value={lastFinishedJob ? humanizeLabel(lastFinishedJob.status) : 'No builds yet'}
                    meta={
                      lastFinishedJob
                        ? formatCompactDateTime(lastFinishedJob.startedAt || lastFinishedJob.requestedAt)
                        : 'Run a profile to populate history'
                    }
                    tone={lastFinishedJob ? buildTone(lastFinishedJob.status) : 'neutral'}
                  />
                </div>

                {buildError ? (
                  <div className="notice notice--error">
                    <div>
                      <strong>Build API error</strong>
                      <p>{buildError}</p>
                    </div>
                  </div>
                ) : null}

                <section className="panel build-profiles-panel">
                  <div className="panel__header panel__header--wrap build-profiles-panel__header">
                    <div>
                      <h2>Build profiles</h2>
                      <p>
                        {filteredProfiles.length === profiles.length
                          ? 'Select a profile to run a build. All profiles are currently listed below.'
                          : `Showing ${filteredProfiles.length} of ${profiles.length} profiles.`}
                      </p>
                    </div>
                    <div className="build-toolbar">
                      <label className="build-search">
                        <SymbolIcon name="search" className="build-search__icon" />
                        <input
                          type="search"
                          value={profileSearch}
                          onChange={(event) => setProfileSearch(event.target.value)}
                          placeholder="Search profiles..."
                        />
                      </label>
                      <button
                        type="button"
                        className={`ghost-button build-filter-button ${buildFilterOpen ? 'build-filter-button--active' : ''}`}
                        onClick={() => setBuildFilterOpen((current) => !current)}
                      >
                        <SymbolIcon name="filter" className="button-icon" />
                        <span>Filters</span>
                      </button>
                    </div>
                  </div>

                  {buildFilterOpen ? (
                    <div className="build-filter-row">
                      <button
                        type="button"
                        className={`filter-chip ${buildCategoryFilter === 'all' ? 'filter-chip--active' : ''}`}
                        onClick={() => setBuildCategoryFilter('all')}
                      >
                        All
                      </button>
                      {buildCategories.map((category) => (
                        <button
                          key={category}
                          type="button"
                          className={`filter-chip ${buildCategoryFilter === category ? 'filter-chip--active' : ''}`}
                          onClick={() => setBuildCategoryFilter(category)}
                        >
                          {humanizeLabel(category)}
                        </button>
                      ))}
                    </div>
                  ) : null}

                  {filteredProfiles.length === 0 ? (
                    <EmptyState
                      title="No profiles match this view"
                      description="Try clearing the search query or choosing a different category."
                    />
                  ) : (
                    <div className="build-grid build-grid--dashboard">
                      {featuredProfile ? (
                        <BuildProfileCard
                          key={featuredProfile.id}
                          profile={featuredProfile}
                          lastJob={lastJobByProfileId.get(featuredProfile.id) ?? null}
                          featured
                          recommended={featuredProfile.id === 'cmake-default'}
                          busy={startingProfileId === featuredProfile.id}
                          hasLiveBuild={hasLiveBuild}
                          onRun={handleRunBuild}
                          onOpenDetails={() => setSelectedProfileInfoId(featuredProfile.id)}
                          onOpenLogs={() => {
                            const job = lastJobByProfileId.get(featuredProfile.id);
                            if (job) {
                              handleSelectBuildJob(job.id);
                            }
                          }}
                        />
                      ) : null}

                      {compactProfiles.map((profile) => (
                        <BuildProfileCard
                          key={profile.id}
                          profile={profile}
                          lastJob={lastJobByProfileId.get(profile.id) ?? null}
                          busy={startingProfileId === profile.id}
                          hasLiveBuild={hasLiveBuild}
                          onRun={handleRunBuild}
                          onOpenDetails={() => setSelectedProfileInfoId(profile.id)}
                          onOpenLogs={() => {
                            const job = lastJobByProfileId.get(profile.id);
                            if (job) {
                              handleSelectBuildJob(job.id);
                            }
                          }}
                        />
                      ))}
                    </div>
                  )}
                </section>
              </div>

              <div className="page-column">
                <RuntimePanel
                  runtime={runtime}
                  error={runtimeError}
                  launching={launchingRuntime}
                  openingFolder={openingRuntimeFolder}
                  stopping={stoppingRuntime}
                  refreshing={runtimeRefreshing}
                  compact
                  onLaunch={handleLaunchRuntime}
                  onOpenFolder={handleOpenRuntimeFolder}
                  onStop={handleStopRuntime}
                  onOpenSettings={() => setActiveTab('settings')}
                />

                <BuildQueuePanel
                  jobs={queuedJobs}
                  selectedJobId={selectedBuildJobId}
                  onSelect={handleSelectBuildJob}
                />

                <BuildHistory
                  jobs={recentJobs}
                  selectedJobId={selectedBuildJobId}
                  onSelect={handleSelectBuildJob}
                  onOpenLogs={handleSelectBuildJob}
                  onRunProfile={(job) => {
                    const profile = profiles.find((candidate) => candidate.id === job.profileId);
                    if (profile) {
                      void handleRunBuild(profile);
                    }
                  }}
                />

                <BuildJobDetails
                  title={selectedBuildJob?.profileLabel ? `${selectedBuildJob.profileLabel}` : 'Build details'}
                  job={selectedBuildJob}
                  onCancel={handleCancelBuild}
                  cancelling={cancellingJobId === selectedBuildJob?.id}
                  compact
                />

                <BuildLogPanel job={selectedBuildJob} compact />
              </div>
            </section>
          ) : null}

          {activeTab === 'builds' && selectedProfileInfo ? (
            <BuildProfileInfoModal
              profile={selectedProfileInfo}
              busy={startingProfileId === selectedProfileInfo.id}
              hasLiveBuild={hasLiveBuild}
              updatingOptionKey={updatingBuildOptionKey}
              onClose={() => setSelectedProfileInfoId('')}
              onRun={handleRunBuild}
              onSetCmakeOption={handleSetBuildOption}
            />
          ) : null}

          {activeTab === 'mods' ? (
            <section className="panel panel--page">
              <div className="panel__header panel__header--wrap">
                <div>
                  <h2>Mods inventory</h2>
                  <p>
                    Best-effort inventory of the current Skyrim Data directory. Plugin-backed mods
                    can be enabled or disabled through Plugins.txt, while other entries are shown
                    as inventory-only for now.
                  </p>
                </div>
                <div className="button-row">
                  <button
                    type="button"
                    className="ghost-button"
                    disabled={modsRefreshing || modsReordering}
                    onClick={() => void refreshMods({ silent: true })}
                  >
                    <SymbolIcon name="refresh" className="button-icon" />
                    <span>{modsRefreshing || modsReordering ? 'Refreshing...' : 'Refresh mods'}</span>
                  </button>
                  <button
                    type="button"
                    className="ghost-button"
                    onClick={() => setActiveTab('settings')}
                  >
                    Settings
                  </button>
                </div>
              </div>

              <ModsSummary snapshot={modsSnapshot} />

              {modsError ? (
                <div className="notice notice--error">
                  <div>
                    <strong>Mods API error</strong>
                    <p>{modsError}</p>
                  </div>
                </div>
              ) : null}

              <div className="inline-summary">
                <strong>Resolved paths</strong>
                <p>
                  Scanning <code>{formatPath(modsSnapshot.dataDir?.path)}</code> and reading{' '}
                  <code>{formatPath(modsSnapshot.pluginsTxt?.path)}</code>.
                </p>
                <p>
                  Plugins.txt status: {modsSnapshot.pluginsTxt?.exists ? 'Found' : 'Missing'}{' '}
                  {modsSnapshot.pluginsTxt?.readable === false
                    ? `(unreadable: ${modsSnapshot.pluginsTxt?.readError})`
                    : ''}
                </p>
                {modsSnapshot.runtime?.blocked ? (
                  <p>{modsSnapshot.runtime.reason}</p>
                ) : null}
              </div>

              <div className="build-toolbar mods-toolbar">
                <label className="build-search">
                  <SymbolIcon name="search" className="build-search__icon" />
                  <input
                    type="search"
                    value={modSearch}
                    onChange={(event) => setModSearch(event.target.value)}
                    placeholder="Search mods, plugins, or paths..."
                  />
                </label>
              </div>

              <div className="filter-row mods-filter-row">
                {[
                  ['all', 'All'],
                  ['enabled', 'Enabled'],
                  ['disabled', 'Disabled'],
                  ['attention', 'Attention'],
                  ['toggleable', 'Toggleable'],
                  ['inventory_only', 'Inventory only'],
                ].map(([value, label]) => (
                  <button
                    key={value}
                    type="button"
                    className={`filter-chip ${modFilter === value ? 'filter-chip--active' : ''}`}
                    onClick={() => setModFilter(value)}
                  >
                    {label}
                  </button>
                ))}
              </div>

              {!modsSnapshot.dataDir?.exists ? (
                <EmptyState
                  title="Skyrim Data directory unavailable"
                  description="Configure a valid Skyrim Root in Settings to populate the Mods tab."
                />
              ) : (
                <ModsInventoryTable
                  mods={filteredMods}
                  togglingModId={togglingModId}
                  reordering={modsReordering}
                  onReorder={handleReorderMods}
                  onToggle={handleToggleMod}
                />
              )}
            </section>
          ) : null}

          {activeTab === 'sync' ? (
            <section className="panel panel--page">
              <div className="panel__header panel__header--wrap">
                <div>
                  <h2>Sync status</h2>
                  <p>
                    Review manifest-driven artifact health, run syncs, and filter to the
                    items that need attention.
                  </p>
                </div>
                <div className="button-row">
                  <button
                    type="button"
                    className="ghost-button"
                    disabled={syncing}
                    onClick={() => void handleSync(true)}
                  >
                    Dry run
                  </button>
                  <button
                    type="button"
                    className="primary-button"
                    disabled={syncing}
                    onClick={() => void handleSync(false)}
                  >
                    {syncing ? 'Syncing...' : 'Sync now'}
                  </button>
                </div>
              </div>

              <SyncSummary summary={statusSummary} />


              <section className="sync-data">
                
              {syncResult ? (
                <div className="inline-summary">
                  <strong>Latest sync result</strong>
                  <p>
                    Touched {syncSummary.touched} artifact pair(s), skipped {syncSummary.skipped},
                    removed {syncSummary.stale} stale file(s).
                  </p>
                </div>
              ) : null}
                <div className="filter-row">
                  {[
                    ['all', 'All'],
                    ['attention', 'Needs attention'],
                    ['dirty', 'Dirty'],
                    ['ok', 'Healthy'],
                    ['skipped', 'Skipped'],
                    ['skipped_no_skyrim', 'Needs config'],
                  ].map(([value, label]) => (
                    <button
                      key={value}
                      type="button"
                      className={`filter-chip ${syncFilter === value ? 'filter-chip--active' : ''}`}
                      onClick={() => setSyncFilter(value)}
                    >
                      {label}
                    </button>
                  ))}
                </div>

                {filteredRows.length === 0 ? (
                  <EmptyState
                    title="No artifacts match this filter"
                    description="Try a different filter or refresh the dashboard."
                  />
                ) : (
                  <div className="table-wrap">
                    <table className="data-table">
                      <thead>
                        <tr>
                          <th>Artifact</th>
                          <th>Status</th>
                          <th>Producer</th>
                          <th>Source</th>
                          <th>Destination</th>
                        </tr>
                      </thead>
                      <tbody>
                        {filteredRows.map((row) => (
                          <tr key={`${row.pair.artifactId}-${row.pair.destPath}`}>
                            <td>
                              <strong>{row.pair.label}</strong>
                            </td>
                            <td>
                              <span className={`pill pill--${statusTone(row.status)}`}>
                                {row.status}
                              </span>
                            </td>
                            <td>{row.pair.producer || 'Unknown'}</td>
                            <td>
                              <code>{row.pair.sourcePath}</code>
                            </td>
                            <td>
                              <code>{row.pair.destPath}</code>
                            </td>
                          </tr>
                        ))}
                      </tbody>
                    </table>
                  </div>
                )}
              </section>
            </section>
          ) : null}

          {activeTab === 'doctor' ? (
            <section className="panel panel--page">
              <div className="panel__header">
                <div>
                  <h2>Doctor</h2>
                  <p>
                    Environment validation for your repo, local paths, and current runtime
                    assumptions.
                  </p>
                </div>
              </div>

              <DoctorSummary issues={doctorIssues} />

              {doctorIssues.length === 0 ? (
                <EmptyState
                  title="Everything looks healthy"
                  description="The doctor command did not report any current problems."
                />
              ) : (
                <div className="issue-list">
                  {doctorIssues.map((issue) => (
                    <article
                      key={`${issue.level}-${issue.code}`}
                      className={`issue-card issue-card--${issue.level}`}
                    >
                      <div className="issue-card__header">
                        <span className={`pill pill--${issue.level}`}>{issue.level}</span>
                        <strong>{issue.code}</strong>
                      </div>
                      <p>{issue.message}</p>
                    </article>
                  ))}
                </div>
              )}
            </section>
          ) : null}

          {activeTab === 'settings' ? (
            <section className="panel panel--page">
              <div className="panel__header panel__header--wrap">
                <div>
                  <h2>Settings</h2>
                  <p>
                    Edit your local dashboard configuration here. Saving will create or
                    update the YAML file automatically.
                  </p>
                </div>
                <div className="button-row">
                  <button
                    type="button"
                    className="ghost-button"
                    disabled={saving}
                    onClick={handleResetSettings}
                  >
                    Reset
                  </button>
                  <button
                    type="button"
                    className="primary-button"
                    disabled={saving}
                    onClick={() => void handleSaveSettings()}
                  >
                    {saving ? 'Saving...' : 'Save settings'}
                  </button>
                </div>
              </div>

              <div className="inline-summary">
                <strong>Config file</strong>
                <p>{configData?.configPath}</p>
                <span className={`pill pill--${meta?.configExists ? 'ok' : 'warn'}`}>
                  {meta?.configExists ? 'Existing file' : 'Will be created on first save'}
                </span>
              </div>

              {(configData?.schema?.sections ?? []).map((section) => (
                <section key={section.id} className="settings-section">
                  <div className="settings-section__header">
                    <h3>{section.title}</h3>
                    <p>{section.description}</p>
                  </div>
                  <div className="settings-grid">
                    {section.fields.map((field) => (
                      <label className="field" key={field.key}>
                        <span className="field__label">{field.label}</span>
                        <input
                          className={`field__input ${saveErrors[field.key] ? 'field__input--error' : ''}`}
                          type={field.input === 'text' ? 'text' : field.input}
                          value={formValues[field.key] ?? ''}
                          placeholder={field.placeholder ?? ''}
                          onChange={(event) =>
                            handleFieldChange(field.key, event.target.value)
                          }
                        />
                        <span className="field__description">{field.description}</span>
                        {saveErrors[field.key] ? (
                          <span className="field__error">{saveErrors[field.key]}</span>
                        ) : null}
                      </label>
                    ))}
                  </div>
                </section>
              ))}
            </section>
          ) : null}
        </>
      )}
    </div>
  );
}
