export interface VoiceActivityPacketEnvelope {
  eventName: 'syncVoiceActivity';
  content: Record<string, unknown>;
}

export function parseSyncVoiceActivityPacket(
  rawJson: string,
): VoiceActivityPacketEnvelope | null {
  const parsed = JSON.parse(rawJson);
  if (!parsed || typeof parsed !== 'object') {
    return null;
  }

  const content = parsed as Record<string, unknown>;
  const packetType = content.customPacketType;
  if (packetType !== 'syncVoiceActivity') {
    return null;
  }

  return {
    eventName: 'syncVoiceActivity',
    content,
  };
}
