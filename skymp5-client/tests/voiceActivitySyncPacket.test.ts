import assert from 'node:assert/strict';
import { parseSyncVoiceActivityPacket } from '../src/services/services/voiceActivitySyncPacket';

function testParsesSyncVoiceActivityPacket(): void {
  const packet = parseSyncVoiceActivityPacket(
    JSON.stringify({
      customPacketType: 'syncVoiceActivity',
      actorId: '0x14',
      isSpeaking: true,
      voiceRange: 15,
      timestamp: Date.now(),
    }),
  );

  assert.ok(packet);
  assert.equal(packet.eventName, 'syncVoiceActivity');
  assert.equal(packet.content.customPacketType, 'syncVoiceActivity');
}

function testIgnoresOtherCustomPacketTypes(): void {
  const packet = parseSyncVoiceActivityPacket(
    JSON.stringify({
      customPacketType: 'invokeAnimResult',
      result: { success: true },
    }),
  );

  assert.equal(packet, null);
}

function testThrowsOnInvalidJson(): void {
  assert.throws(() => parseSyncVoiceActivityPacket('{invalid-json}'));
}

function run(): void {
  testParsesSyncVoiceActivityPacket();
  testIgnoresOtherCustomPacketTypes();
  testThrowsOnInvalidJson();
  console.log('voiceActivitySyncPacket tests passed');
}

run();
