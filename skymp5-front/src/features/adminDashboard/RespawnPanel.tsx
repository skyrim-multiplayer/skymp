/**
 * Admin Dashboard Respawn & Revival Configuration Panel
 */
import React from 'react';
import { useTranslation } from 'react-i18next';

import { formatAdminUptime } from './utils';

export interface DownedPlayerEntry {
  userId: number;
  actorName: string;
  downedAt: number;
  canRespawn: boolean;
}

interface RespawnPanelProps {
  downedPlayers: DownedPlayerEntry[];
  moderationReason: string;
  onModerationReasonChange: (reason: string) => void;
  onRevivePlayer: (userId: number) => void;
  nowTs: number;
  statusMsg: string;
  loading: boolean;
}

export const RespawnPanel: React.FC<RespawnPanelProps> = ({
  downedPlayers,
  moderationReason,
  onModerationReasonChange,
  onRevivePlayer,
  nowTs,
  statusMsg,
  loading,
}) => {
  const { t } = useTranslation();

  return (
    <div
      className="admin-dashboard__panel"
      id="admin-panel-respawn"
      role="tabpanel"
      aria-labelledby="admin-tab-respawn"
    >
      <h2 className="admin-dashboard__panel-title">
        {t('adminDashboard.respawnConfigTitle')}
      </h2>

      <div className="admin-dashboard__respawn-info">
        <div className="admin-dashboard__respawn-stat">
          <span className="admin-dashboard__respawn-label">Downed Players</span>
          <span className="admin-dashboard__respawn-value">
            {downedPlayers.length}
          </span>
        </div>
      </div>

      {downedPlayers.length === 0 ? (
        <p className="admin-dashboard__no-players">
          {t('adminDashboard.noDownedPlayers') || 'No downed players'}
        </p>
      ) : (
        <>
          <div className="admin-dashboard__search-row">
            <input
              className="admin-dashboard__search-input admin-dashboard__search-input--reason"
              type="text"
              placeholder={t('adminDashboard.reasonPlaceholder')}
              aria-label="Revival reason"
              value={moderationReason}
              onChange={(e) => onModerationReasonChange(e.target.value)}
            />
          </div>

          <div className="admin-dashboard__downed-list">
            {downedPlayers.map((player) => {
              const downtimeSec = Math.floor((nowTs - player.downedAt) / 1000);
              return (
                <div
                  key={player.userId}
                  className="admin-dashboard__downed-row"
                  data-testid={`admin-downed-row-${player.userId}`}
                >
                  <div className="admin-dashboard__downed-info">
                    <span className="admin-dashboard__downed-name">
                      {player.actorName} (User ID: {player.userId})
                    </span>
                    <span className="admin-dashboard__downed-time">
                      Downed for: {formatAdminUptime(downtimeSec)}
                    </span>
                    <span
                      className={`admin-dashboard__downed-status ${
                        player.canRespawn ? 'can-respawn' : 'no-respawn'
                      }`}
                    >
                      {player.canRespawn
                        ? '✓ Auto-respawn enabled'
                        : '✗ Auto-respawn disabled'}
                    </span>
                  </div>
                  <button
                    type="button"
                    className="admin-dashboard__revive-btn"
                    data-testid={`admin-revive-btn-${player.userId}`}
                    onClick={() => onRevivePlayer(player.userId)}
                    disabled={loading}
                  >
                    {t('adminDashboard.revive') || 'Revive'}
                  </button>
                </div>
              );
            })}
          </div>
        </>
      )}

      {statusMsg && (
        <div
          className="admin-dashboard__status-msg"
          data-testid="admin-status-msg"
          role="status"
          aria-live="polite"
        >
          {statusMsg}
        </div>
      )}
    </div>
  );
};
