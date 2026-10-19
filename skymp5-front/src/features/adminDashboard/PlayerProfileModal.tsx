import React, { useCallback, useEffect, useRef, useState } from 'react';
import { useTranslation } from 'react-i18next';

export interface PlayerProfileData {
  ok: boolean;
  userId: number;
  displayName: string;
  firstJoinedAt: number;
  lastConnectionAt: number;
  playTimeSec: number;
  online: boolean;
  identifiers: string[];
  notes: string;
  history: ProfileHistoryEntry[];
}

export interface ProfileHistoryEntry {
  id: string;
  type: 'warn' | 'ban' | 'kick' | 'mute';
  playerName: string;
  userId: number;
  reason: string;
  author: string;
  ts: number;
}

interface Props {
  userId: number;
  playerName: string;
  onClose: () => void;
  canBan?: boolean;
  canWarn?: boolean;
  onKick?: (userId: number) => void;
  onBan?: (userId: number) => void;
  onWarn?: (userId: number, name: string) => void;
}

type ProfileTab = 'identifiers' | 'history' | 'notes';

const SUPPORTED_IDENTIFIER_TYPES = ['steam', 'discord', 'license', 'licenseea', 'live', 'xblive'];

const formatPlayTime = (sec: number): string => {
  if (sec < 60) return `${sec}s`;
  if (sec < 3600) return `${Math.floor(sec / 60)}m`;
  const h = Math.floor(sec / 3600);
  const m = Math.floor((sec % 3600) / 60);
  return `${h}h ${m}m`;
};

const ACTION_COLORS: Record<ProfileHistoryEntry['type'], string> = {
  ban: '#e05555',
  warn: '#e6b800',
  kick: '#e07b2a',
  mute: '#8888cc',
};

const extractSteamId64 = (identifier: string): string | null => {
  const lower = identifier.trim().toLowerCase();
  if (!lower.startsWith('steam:')) return null;
  const hex = lower.slice('steam:'.length);
  if (!/^[0-9a-f]+$/.test(hex)) return null;
  try {
    return String(BigInt('0x' + hex));
  } catch {
    return null;
  }
};

export const PlayerProfileModal: React.FC<Props> = ({
  userId,
  playerName,
  onClose,
  canBan,
  canWarn,
  onKick,
  onBan,
  onWarn,
}) => {
  const { t } = useTranslation();
  const [activeTab, setActiveTab] = useState<ProfileTab>('identifiers');
  const [profile, setProfile] = useState<PlayerProfileData | null>(null);
  const [loadError, setLoadError] = useState('');
  const [loading, setLoading] = useState(true);

  // Identifiers editing state
  const [editingIdentifiers, setEditingIdentifiers] = useState<string[]>([]);
  const [identifierType, setIdentifierType] = useState('steam');
  const [identifierValue, setIdentifierValue] = useState('');
  const [identifierError, setIdentifierError] = useState('');
  const [saving, setSaving] = useState(false);

  // Notes editing state
  const [notesText, setNotesText] = useState('');
  const [notesDirty, setNotesDirty] = useState(false);
  const [notesSaving, setNotesSaving] = useState(false);

  const notesRef = useRef<HTMLTextAreaElement>(null);

  const fetchProfile = useCallback(async () => {
    setLoading(true);
    setLoadError('');
    try {
      const res = await fetch(`/api/admin/players/${userId}/profile`);
      if (!res.ok) {
        setLoadError(t('adminDashboard.apiError'));
        return;
      }
      const data: PlayerProfileData = await res.json();
      setProfile(data);
      setEditingIdentifiers(data.identifiers ?? []);
      setNotesText(data.notes ?? '');
    } catch {
      setLoadError(t('adminDashboard.apiError'));
    } finally {
      setLoading(false);
    }
  }, [userId, t]);

  useEffect(() => {
    fetchProfile();
  }, [fetchProfile]);

  const saveProfile = async (
    identifiers: string[],
    notes: string,
  ): Promise<boolean> => {
    try {
      const res = await fetch(`/api/admin/players/${userId}/profile`, {
        method: 'PUT',
        headers: { 'Content-Type': 'application/json' },
        body: JSON.stringify({ identifiers, notes }),
      });
      return res.ok;
    } catch {
      return false;
    }
  };

  const handleAddIdentifier = async () => {
    setIdentifierError('');
    const value = identifierValue.trim();
    if (!value) {
      setIdentifierError(
        t('adminDashboard.profileIdentifierValueRequired', {
          defaultValue: 'Wert darf nicht leer sein.',
        }),
      );
      return;
    }
    const combined = `${identifierType}:${value}`;
    if (editingIdentifiers.includes(combined)) {
      setIdentifierError(
        t('adminDashboard.profileIdentifierDuplicate', {
          defaultValue: 'Identifier bereits vorhanden.',
        }),
      );
      return;
    }
    const next = [...editingIdentifiers, combined];
    setSaving(true);
    const ok = await saveProfile(next, notesText);
    setSaving(false);
    if (!ok) {
      setIdentifierError(t('adminDashboard.apiError'));
      return;
    }
    setEditingIdentifiers(next);
    setIdentifierValue('');
  };

  const handleRemoveIdentifier = async (id: string) => {
    const next = editingIdentifiers.filter((v) => v !== id);
    setSaving(true);
    const ok = await saveProfile(next, notesText);
    setSaving(false);
    if (ok) setEditingIdentifiers(next);
  };

  const handleSaveNotes = async () => {
    setNotesSaving(true);
    const ok = await saveProfile(editingIdentifiers, notesText);
    setNotesSaving(false);
    if (ok) setNotesDirty(false);
  };

  const backdrop = (e: React.MouseEvent) => {
    if (e.target === e.currentTarget) onClose();
  };

  return (
    <div
      className="admin-dashboard__warn-modal-backdrop"
      style={{ zIndex: 1100 }}
      onClick={backdrop}
    >
      <div
        className="admin-dashboard__warn-modal"
        style={{ width: 600, maxWidth: '95vw', maxHeight: '85vh', display: 'flex', flexDirection: 'column' }}
        onClick={(e) => e.stopPropagation()}
      >
        {/* Header */}
        <div className="admin-dashboard__warn-modal-head" style={{ flexShrink: 0 }}>
          <div>
            <h3 style={{ margin: 0 }}>
              {t('adminDashboard.profileTitle', { defaultValue: 'Spielerprofil' })}
            </h3>
            <div style={{ fontSize: 12, opacity: 0.7, marginTop: 2 }}>
              {playerName} &nbsp;·&nbsp; userId={userId}
            </div>
          </div>
          <div style={{ display: 'flex', gap: 6 }}>
            {canWarn && onWarn && (
              <button
                type="button"
                style={{ color: '#e6b800', borderColor: '#e6b800' }}
                onClick={() => { onWarn(userId, playerName); onClose(); }}
              >
                {t('adminDashboard.warn', { defaultValue: 'Warnen' })}
              </button>
            )}
            {canBan && onBan && (
              <button
                type="button"
                style={{ color: '#e05555', borderColor: '#e05555' }}
                onClick={() => { onBan(userId); onClose(); }}
              >
                {t('adminDashboard.ban', { defaultValue: 'Ban' })}
              </button>
            )}
            {onKick && (
              <button type="button" onClick={() => { onKick(userId); onClose(); }}>
                {t('adminDashboard.kick', { defaultValue: 'Kick' })}
              </button>
            )}
            <button type="button" onClick={onClose}>×</button>
          </div>
        </div>

        {/* Summary row */}
        {profile && (
          <div
            style={{
              display: 'flex',
              gap: 16,
              padding: '8px 16px',
              background: 'rgba(255,255,255,0.04)',
              borderBottom: '1px solid rgba(255,255,255,0.08)',
              flexShrink: 0,
              flexWrap: 'wrap',
              fontSize: 12,
            }}
          >
            <span>
              <b>{t('adminDashboard.profileStatus', { defaultValue: 'Status' })}: </b>
              <span style={{ color: profile.online ? '#7ec87e' : '#aaa' }}>
                {profile.online
                  ? t('adminDashboard.online', { defaultValue: 'Online' })
                  : t('adminDashboard.offline', { defaultValue: 'Offline' })}
              </span>
            </span>
            <span>
              <b>{t('adminDashboard.profilePlayTime', { defaultValue: 'Spielzeit' })}: </b>
              {formatPlayTime(profile.playTimeSec)}
            </span>
            <span>
              <b>{t('adminDashboard.profileFirstJoined', { defaultValue: 'Beigetreten' })}: </b>
              {new Date(profile.firstJoinedAt).toLocaleDateString()}
            </span>
            <span>
              <b>{t('adminDashboard.profileLastSeen', { defaultValue: 'Zuletzt gesehen' })}: </b>
              {new Date(profile.lastConnectionAt).toLocaleDateString()}
            </span>
            <span>
              <b>{t('adminDashboard.profileActions', { defaultValue: 'Aktionen' })}: </b>
              <span style={{ color: '#e05555' }}>
                {profile.history.filter((h) => h.type === 'ban').length} Bans
              </span>
              {' · '}
              <span style={{ color: '#e6b800' }}>
                {profile.history.filter((h) => h.type === 'warn').length} Warns
              </span>
              {' · '}
              <span style={{ color: '#e07b2a' }}>
                {profile.history.filter((h) => h.type === 'kick').length} Kicks
              </span>
            </span>
          </div>
        )}

        {/* Tabs */}
        <div
          style={{
            display: 'flex',
            gap: 0,
            borderBottom: '1px solid rgba(255,255,255,0.1)',
            flexShrink: 0,
          }}
        >
          {(['identifiers', 'history', 'notes'] as ProfileTab[]).map((tab) => (
            <button
              key={tab}
              type="button"
              style={{
                padding: '8px 16px',
                background: 'none',
                border: 'none',
                borderBottom: activeTab === tab ? '2px solid #7ec8c8' : '2px solid transparent',
                color: activeTab === tab ? '#7ec8c8' : '#aaa',
                cursor: 'pointer',
                fontWeight: activeTab === tab ? 600 : 400,
              }}
              onClick={() => setActiveTab(tab)}
            >
              {tab === 'identifiers' &&
                t('adminDashboard.profileTabIdentifiers', { defaultValue: 'Identifiers' })}
              {tab === 'history' &&
                t('adminDashboard.profileTabHistory', { defaultValue: 'Verlauf' })}
              {tab === 'notes' &&
                t('adminDashboard.profileTabNotes', { defaultValue: 'Notizen' })}
            </button>
          ))}
        </div>

        {/* Body */}
        <div style={{ flex: 1, overflowY: 'auto', padding: 16 }}>
          {loading && (
            <div style={{ color: '#aaa' }}>
              {t('adminDashboard.loading', { defaultValue: 'Laden...' })}
            </div>
          )}
          {loadError && <div style={{ color: '#e05555' }}>{loadError}</div>}

          {/* IDENTIFIERS TAB */}
          {!loading && !loadError && activeTab === 'identifiers' && (
            <div>
              <div style={{ marginBottom: 12, fontSize: 12, opacity: 0.7 }}>
                {t('adminDashboard.profileIdentifiersHint', {
                  defaultValue:
                    'Steam IDs, Discord IDs und andere Identifier manuell hinterlegen. Format: typ:wert (z.B. steam:110000104e4b93c)',
                })}
              </div>

              {/* Add new identifier */}
              <div style={{ display: 'flex', gap: 6, marginBottom: 12, alignItems: 'flex-start', flexWrap: 'wrap' }}>
                <select
                  value={identifierType}
                  onChange={(e) => setIdentifierType(e.target.value)}
                  disabled={saving}
                  style={{ height: 32 }}
                >
                  {SUPPORTED_IDENTIFIER_TYPES.map((type) => (
                    <option key={type} value={type}>{type}</option>
                  ))}
                </select>
                <input
                  type="text"
                  value={identifierValue}
                  onChange={(e) => { setIdentifierValue(e.target.value); setIdentifierError(''); }}
                  placeholder={
                    identifierType === 'steam'
                      ? '110000104e4b93c'
                      : identifierType === 'discord'
                        ? '123456789012345678'
                        : 'Wert...'
                  }
                  disabled={saving}
                  onKeyDown={(e) => { if (e.key === 'Enter') handleAddIdentifier(); }}
                  style={{ flex: 1, minWidth: 160, height: 32 }}
                />
                <button
                  type="button"
                  disabled={saving || !identifierValue.trim()}
                  onClick={handleAddIdentifier}
                  style={{ height: 32 }}
                >
                  {t('adminDashboard.profileAddIdentifier', { defaultValue: '+ Hinzufügen' })}
                </button>
              </div>
              {identifierError && (
                <div style={{ color: '#e05555', fontSize: 12, marginBottom: 8 }}>
                  {identifierError}
                </div>
              )}

              {/* Existing identifiers */}
              {editingIdentifiers.length === 0 ? (
                <div style={{ color: '#888', fontSize: 13 }}>
                  {t('adminDashboard.profileNoIdentifiers', {
                    defaultValue: 'Noch keine Identifier hinterlegt.',
                  })}
                </div>
              ) : (
                <table style={{ width: '100%', fontSize: 13, borderCollapse: 'collapse' }}>
                  <thead>
                    <tr style={{ opacity: 0.6 }}>
                      <th style={{ textAlign: 'left', padding: '4px 8px' }}>Typ</th>
                      <th style={{ textAlign: 'left', padding: '4px 8px' }}>Wert</th>
                      <th style={{ textAlign: 'left', padding: '4px 8px' }}>Link</th>
                      <th style={{ padding: '4px 8px' }}></th>
                    </tr>
                  </thead>
                  <tbody>
                    {editingIdentifiers.map((id) => {
                      const colonIdx = id.indexOf(':');
                      const type = colonIdx > 0 ? id.slice(0, colonIdx) : id;
                      const value = colonIdx > 0 ? id.slice(colonIdx + 1) : '';
                      const steamId64 = type === 'steam' ? extractSteamId64(id) : null;
                      return (
                        <tr
                          key={id}
                          style={{ borderBottom: '1px solid rgba(255,255,255,0.06)' }}
                        >
                          <td style={{ padding: '6px 8px', fontWeight: 600, color: '#7ec8c8' }}>
                            {type}
                          </td>
                          <td
                            style={{
                              padding: '6px 8px',
                              fontFamily: 'monospace',
                              wordBreak: 'break-all',
                            }}
                          >
                            {value}
                            {steamId64 && (
                              <div style={{ fontSize: 11, opacity: 0.6 }}>
                                SteamID64: {steamId64}
                              </div>
                            )}
                          </td>
                          <td style={{ padding: '6px 8px' }}>
                            {steamId64 && (
                              <a
                                href={`https://steamcommunity.com/profiles/${steamId64}`}
                                target="_blank"
                                rel="noreferrer noopener"
                                style={{ fontSize: 12, color: '#7ec8c8' }}
                              >
                                Steam↗
                              </a>
                            )}
                            {type === 'discord' && (
                              <span style={{ fontSize: 12, color: '#7b8cde' }}>Discord</span>
                            )}
                          </td>
                          <td style={{ padding: '6px 8px' }}>
                            <button
                              type="button"
                              disabled={saving}
                              onClick={() => handleRemoveIdentifier(id)}
                              style={{ color: '#e05555', border: 'none', background: 'none', cursor: 'pointer', fontSize: 16 }}
                              title={t('adminDashboard.profileRemoveIdentifier', { defaultValue: 'Entfernen' })}
                            >
                              ×
                            </button>
                          </td>
                        </tr>
                      );
                    })}
                  </tbody>
                </table>
              )}
            </div>
          )}

          {/* HISTORY TAB */}
          {!loading && !loadError && activeTab === 'history' && (
            <div>
              {profile && profile.history.length === 0 ? (
                <div style={{ color: '#888', fontSize: 13 }}>
                  {t('adminDashboard.profileNoHistory', {
                    defaultValue: 'Keine Moderationsaktionen vorhanden.',
                  })}
                </div>
              ) : (
                <table style={{ width: '100%', fontSize: 13, borderCollapse: 'collapse' }}>
                  <thead>
                    <tr style={{ opacity: 0.6 }}>
                      <th style={{ textAlign: 'left', padding: '4px 8px' }}>Typ</th>
                      <th style={{ textAlign: 'left', padding: '4px 8px' }}>Grund</th>
                      <th style={{ textAlign: 'left', padding: '4px 8px' }}>Admin</th>
                      <th style={{ textAlign: 'left', padding: '4px 8px' }}>Datum</th>
                    </tr>
                  </thead>
                  <tbody>
                    {profile?.history.map((entry) => (
                      <tr
                        key={entry.id}
                        style={{ borderBottom: '1px solid rgba(255,255,255,0.06)' }}
                      >
                        <td style={{ padding: '6px 8px' }}>
                          <span
                            style={{
                              color: ACTION_COLORS[entry.type],
                              fontWeight: 700,
                              textTransform: 'uppercase',
                              fontSize: 11,
                            }}
                          >
                            {entry.type}
                          </span>
                        </td>
                        <td style={{ padding: '6px 8px', wordBreak: 'break-word' }}>
                          {entry.reason || '-'}
                        </td>
                        <td style={{ padding: '6px 8px', opacity: 0.8 }}>{entry.author}</td>
                        <td style={{ padding: '6px 8px', opacity: 0.7, whiteSpace: 'nowrap' }}>
                          {new Date(entry.ts).toLocaleString()}
                        </td>
                      </tr>
                    ))}
                  </tbody>
                </table>
              )}
            </div>
          )}

          {/* NOTES TAB */}
          {!loading && !loadError && activeTab === 'notes' && (
            <div style={{ display: 'flex', flexDirection: 'column', gap: 8 }}>
              <div style={{ fontSize: 12, opacity: 0.6 }}>
                {t('adminDashboard.profileNotesHint', {
                  defaultValue: 'Interne Notizen für das Admin-Team. Nur für Admins sichtbar.',
                })}
              </div>
              <textarea
                ref={notesRef}
                value={notesText}
                onChange={(e) => { setNotesText(e.target.value); setNotesDirty(true); }}
                disabled={notesSaving}
                rows={10}
                maxLength={2000}
                placeholder={t('adminDashboard.profileNotesPlaceholder', {
                  defaultValue: 'Notizen hinzufügen...',
                })}
                style={{
                  width: '100%',
                  resize: 'vertical',
                  padding: 8,
                  background: 'rgba(0,0,0,0.3)',
                  border: '1px solid rgba(255,255,255,0.15)',
                  color: '#fff',
                  borderRadius: 4,
                  fontFamily: 'inherit',
                  fontSize: 13,
                  boxSizing: 'border-box',
                }}
              />
              <div style={{ display: 'flex', justifyContent: 'space-between', alignItems: 'center' }}>
                <span style={{ fontSize: 12, opacity: 0.5 }}>
                  {notesText.length}/2000
                </span>
                <button
                  type="button"
                  disabled={!notesDirty || notesSaving}
                  onClick={handleSaveNotes}
                >
                  {notesSaving
                    ? t('adminDashboard.saving', { defaultValue: 'Speichern...' })
                    : t('adminDashboard.save', { defaultValue: 'Speichern' })}
                </button>
              </div>
            </div>
          )}
        </div>
      </div>
    </div>
  );
};
