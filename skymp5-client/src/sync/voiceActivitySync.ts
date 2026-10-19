import { Actor, Game, Idle } from 'skyrimPlatform';

interface VoiceActivityBroadcast {
  actorId: string;
  isSpeaking: boolean;
  voiceRange: number;
  providerId?: string;
  timestamp: number;
}

interface VoicePacketEnvelope {
  eventName?: string;
  content?: unknown;
}

const LIP_SYNC_ANIMATIONS = {
  IDLE_DIALOGUE_LOCK: 0x0005b1b0,
};

const activeAnimations = new Map<string, ReturnType<typeof setTimeout>>();

function parseActorId(rawActorId: string): number {
  const trimmed = rawActorId.trim();
  if (trimmed.startsWith('0x') || trimmed.startsWith('0X')) {
    return Number.parseInt(trimmed.slice(2), 16);
  }
  return Number(trimmed);
}

function asVoiceBroadcast(payload: unknown): VoiceActivityBroadcast | null {
  if (!payload || typeof payload !== 'object') return null;
  const candidate = payload as Partial<VoiceActivityBroadcast>;
  if (typeof candidate.actorId !== 'string') return null;
  if (typeof candidate.isSpeaking !== 'boolean') return null;
  if (typeof candidate.voiceRange !== 'number') return null;
  if (typeof candidate.timestamp !== 'number') return null;
  if (
    typeof candidate.providerId !== 'undefined' &&
    typeof candidate.providerId !== 'string'
  )
    return null;
  return candidate as VoiceActivityBroadcast;
}

export function initializeClientVoiceSync(
  registerCustomPacketListener?: (handler: (packet: unknown) => void) => void,
): void {
  console.log('[Voice Activity Client] Initializing voice sync...');

  if (!registerCustomPacketListener) {
    console.log(
      '[Voice Activity Client] No custom packet listener registrar provided, voice sync is idle.',
    );
    return;
  }

  registerCustomPacketListener((packet: unknown) => {
    const envelope = packet as VoicePacketEnvelope;
    if (envelope.eventName !== 'syncVoiceActivity') return;

    const broadcast = asVoiceBroadcast(envelope.content ?? packet);
    if (!broadcast) return;

    handleVoiceActivityBroadcast(broadcast);
  });

  console.log('[Voice Activity Client] Voice sync initialized');
}

function handleVoiceActivityBroadcast(broadcast: VoiceActivityBroadcast): void {
  const actorFormId = parseActorId(broadcast.actorId);
  if (!Number.isFinite(actorFormId)) {
    console.warn(
      `[Voice Activity Client] Invalid actorId payload: ${broadcast.actorId}`,
    );
    return;
  }

  const actor = Actor.from(Game.getFormEx(actorFormId));
  if (!actor) {
    console.warn(`[Voice Activity Client] Actor not found: ${broadcast.actorId}`);
    return;
  }

  try {
    const existingTimer = activeAnimations.get(broadcast.actorId);
    if (existingTimer) {
      clearTimeout(existingTimer);
      activeAnimations.delete(broadcast.actorId);
    }

    if (broadcast.isSpeaking) {
      applyLipSyncAnimation(actor, broadcast.actorId);
    } else {
      clearLipSyncAnimation(actor);
    }

    console.log(
      `[Voice Activity Client] ${actor.getName()} (${broadcast.actorId}): ${
        broadcast.isSpeaking ? 'speaking' : 'quiet'
      } (range: ${broadcast.voiceRange}m)`,
    );
  } catch (err) {
    console.error('[Voice Activity Client] Error handling broadcast:', err);
  }
}

function applyLipSyncAnimation(actor: Actor, actorIdKey: string): void {
  const lockForm = Game.getFormEx(LIP_SYNC_ANIMATIONS.IDLE_DIALOGUE_LOCK);
  if (!lockForm) {
    console.warn('[Voice Activity Client] Could not find IdleDialogueLock');
    return;
  }

  actor.playIdle(lockForm as Idle);

  const scheduleRefresh = (): void => {
    const refreshTimer = setTimeout(() => {
      if (!activeAnimations.has(actorIdKey)) return;

      actor.playIdle(lockForm as Idle);
      scheduleRefresh();
    }, 2000);

    activeAnimations.set(actorIdKey, refreshTimer);
  };

  scheduleRefresh();
}

function clearLipSyncAnimation(actor: Actor): void {
  try {
    actor.stopTranslation();
  } catch (err) {
    console.error('[Voice Activity Client] Error clearing animation:', err);
  }
}

export function shutdownClientVoiceSync(): void {
  console.log('[Voice Activity Client] Shutting down voice sync...');

  activeAnimations.forEach((timer) => {
    clearTimeout(timer);
  });
  activeAnimations.clear();

  console.log('[Voice Activity Client] Shutdown complete');
}

export interface VoiceEffect {
  type: 'muffled' | 'reverb' | 'echo' | 'distortion' | 'racial';
  intensity: number;
}

export function applyVoiceEffect(_actor: Actor, effect: VoiceEffect): void {
  console.log(
    `[Voice Activity Client] Voice effect ${effect.type} would be applied`,
  );
}
