/**
 * Admin Dashboard Revival Events Log Panel
 */
import React from 'react';
import { useTranslation } from 'react-i18next';

import { formatAdminTime } from './utils';

export interface RevivalEventEntry {
  ts: number;
  type:
    | 'downed'
    | 'revived'
    | 'respawn_disabled'
    | 'respawn_enabled'
    | 'auto_revived';
  userId: number;
  actorName?: string;
  details?: string;
}

interface EventsPanelProps {
  revivalEvents: RevivalEventEntry[];
  eventTypeFilter:
    | ''
    | 'downed'
    | 'revived'
    | 'respawn_disabled'
    | 'respawn_enabled'
    | 'auto_revived';
  onEventTypeFilterChange: (
    filter:
      | ''
      | 'downed'
      | 'revived'
      | 'respawn_disabled'
      | 'respawn_enabled'
      | 'auto_revived',
  ) => void;
  eventLimit: number;
  onEventLimitChange: (limit: number) => void;
  onRefresh: () => void;
  loading: boolean;
}

export const EventsPanel: React.FC<EventsPanelProps> = ({
  revivalEvents,
  eventTypeFilter,
  onEventTypeFilterChange,
  eventLimit,
  onEventLimitChange,
  onRefresh,
  loading,
}) => {
  const { t } = useTranslation();

  const getEventTypeColor = (type: RevivalEventEntry['type']): string => {
    switch (type) {
      case 'downed':
        return 'admin-dashboard__event-type--downed';
      case 'revived':
        return 'admin-dashboard__event-type--revived';
      case 'respawn_disabled':
        return 'admin-dashboard__event-type--disabled';
      case 'respawn_enabled':
        return 'admin-dashboard__event-type--enabled';
      case 'auto_revived':
        return 'admin-dashboard__event-type--auto';
      default:
        return '';
    }
  };

  const getEventTypeLabel = (type: RevivalEventEntry['type']): string => {
    switch (type) {
      case 'downed':
        return 'Downed';
      case 'revived':
        return 'Revived';
      case 'respawn_disabled':
        return 'Respawn Disabled';
      case 'respawn_enabled':
        return 'Respawn Enabled';
      case 'auto_revived':
        return 'Auto-Revived';
      default:
        return type;
    }
  };

  return (
    <div
      className="admin-dashboard__panel"
      id="admin-panel-events"
      role="tabpanel"
      aria-labelledby="admin-tab-events"
    >
      <h2 className="admin-dashboard__panel-title">
        {t('adminDashboard.revivalEventsTitle') || 'Revival Events Log'}
      </h2>

      <div className="admin-dashboard__log-controls">
        <select
          className="admin-dashboard__log-select"
          value={eventTypeFilter}
          aria-label="Event type filter"
          onChange={(e) => onEventTypeFilterChange(e.target.value as any)}
        >
          <option value="">All Event Types</option>
          <option value="downed">Downed</option>
          <option value="revived">Revived</option>
          <option value="respawn_disabled">Respawn Disabled</option>
          <option value="respawn_enabled">Respawn Enabled</option>
          <option value="auto_revived">Auto-Revived</option>
        </select>

        <select
          className="admin-dashboard__log-select"
          value={eventLimit}
          aria-label="Event limit"
          onChange={(e) => onEventLimitChange(Number(e.target.value))}
        >
          {[25, 50, 100, 200].map((value) => (
            <option key={value} value={value}>
              {value}
            </option>
          ))}
        </select>

        <button
          type="button"
          className="admin-dashboard__log-page-btn"
          onClick={onRefresh}
          disabled={loading}
        >
          {t('adminDashboard.refresh') || 'Refresh'}
        </button>
      </div>

      {revivalEvents.length === 0 ? (
        <p className="admin-dashboard__no-players">
          {t('adminDashboard.noRevivalEvents') || 'No revival events'}
        </p>
      ) : (
        <div className="admin-dashboard__log-list">
          {revivalEvents.map((event, index) => (
            <div
              key={`${event.userId}-${event.ts}-${index}`}
              className="admin-dashboard__log-entry"
              data-testid={`admin-event-entry-${event.type}-${index}`}
            >
              <span className="admin-dashboard__log-ts">
                {formatAdminTime(event.ts)}
              </span>
              <span
                className={`admin-dashboard__log-type admin-dashboard__event-type ${getEventTypeColor(
                  event.type,
                )}`}
              >
                {getEventTypeLabel(event.type)}
              </span>
              <span className="admin-dashboard__log-msg">
                User {event.userId}
                {event.actorName ? ` (${event.actorName})` : ''}
                {event.details ? ` - ${event.details}` : ''}
              </span>
            </div>
          ))}
        </div>
      )}
    </div>
  );
};
