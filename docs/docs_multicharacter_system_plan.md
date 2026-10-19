# Multi-Character System Implementation Plan

## Goal

Implement a production-ready multi-character flow where a player:

1. Authenticates once with their account.
2. Sees a character selection screen.
3. Chooses an existing character or creates a new one.
4. Spawns into the world with the selected character.

The system must stay server-authoritative and secure against client-side spoofing.

## Current Baseline

- Login already exists and emits `spawnAllowed` with `profile.id`.
- Spawn currently auto-selects only the first actor for a profile.
- Character creation currently happens implicitly when no actor exists.
- Client auth flow already handles custom packets and browser UI integration.

## Scope

### In Scope

- Character list retrieval and presentation.
- Character selection packet and validation.
- Character creation packet and server-side creation path.
- Spawn pipeline changes to support explicit selected character.
- Configurable per-profile character cap.
- Minimal admin/telemetry logging for actions.

### Out of Scope (Initial Version)

- Character deletion UI.
- Cross-account character transfer.
- Name-change tooling.
- Paid slot expansion.
- Deep cosmetic preview in selection screen.

## Architecture

## Data Model

Store character metadata on actor records (`mp.set(...)`), for example:

- `private.charName: string`
- `private.charSlot: number`
- `private.createdAt: number` (unix ms)
- `private.lastPlayedAt: number` (unix ms)

No trust in client-provided metadata. Server computes canonical list from actors bound to `profileId`.

## Server Events and Packets

### Server -> Client

- `characterList`
  - `characters: Array<{ actorId: number; name: string; level: number; slot: number; lastPlayedAt?: number }>`
  - `canCreateNew: boolean`
  - `maxCharacters: number`

- `characterSelectRejected`
  - `reason: string`

- `characterCreateRejected`
  - `reason: string`

### Client -> Server

- `selectCharacter`
  - `actorId: number`

- `createCharacter`
  - `name: string`

## Security Rules (Must Have)

1. On `selectCharacter`, verify that `actorId` is inside `ctx.svr.getActorsByProfileId(profileId)`.
2. Reject selection if actor is disabled/deleted/invalid.
3. On `createCharacter`, enforce server-side constraints:
   - max character count
   - name format and length
   - name uniqueness policy (if enabled)
4. Never accept client-side `profileId`, level, or inventory values.
5. Re-check connection/user identity before final spawn (`ctx.svr.isConnected(userId)`).

## Integration Plan

### Phase 1 - Server Core

Target files:

- `skymp5-server/ts/systems/spawn.ts`
- New: `skymp5-server/ts/systems/characterSelect.ts`

Tasks:

1. Refactor spawn listener so it does not auto-pick `[0]` when multiple characters exist.
2. Emit `characterList` when `actors.length > 0`.
3. Keep direct creation path when no actors exist.
4. Add `characterSelect.ts` system to handle `selectCharacter` and `createCharacter` packets.
5. Wire system into server initialization order (after login, before final spawn handoff).

Acceptance criteria:

- Existing profile with 2+ actors gets a list instead of immediate spawn.
- Valid `selectCharacter` spawns chosen actor.
- Invalid selection is rejected with explicit reason.

### Phase 2 - Client Flow

Target files:

- `skymp5-client/src/services/services/authService.ts`
- Frontend selection view module (new component in `skymp5-front` or existing login webview flow)

Tasks:

1. Handle `characterList` packet and show selection UI.
2. Send `selectCharacter` packet on click.
3. Send `createCharacter` packet from "Create" action.
4. Block duplicate submits while pending.
5. Surface rejection reasons in UI.

Acceptance criteria:

- Player can select an existing character from UI.
- Player can create a new character if below cap.
- UI recovers gracefully from rejection/timeouts.

### Phase 3 - Character Metadata and UX

Tasks:

1. Save/update `private.lastPlayedAt` on successful spawn.
2. Display sorted list by slot or last played timestamp.
3. Add optional quick info (level/location if cheap to resolve).

Acceptance criteria:

- Character list ordering is deterministic.
- Last played timestamp updates after join.

### Phase 4 - Hardening and Observability

Tasks:

1. Add structured logs for selection/create success and reject reasons.
2. Add rate limit (simple per-user cooldown) on `createCharacter`.
3. Add metrics counters for:
   - list_shown
   - select_success
   - select_reject
   - create_success
   - create_reject

Acceptance criteria:

- Common abuse attempts are rejected and logged.
- Operational metrics exist for rollout monitoring.

## Server Settings

Add a settings block in `server-settings.json`:

```json
{
  "characterSystem": {
    "maxCharactersPerProfile": 5,
    "nameMinLength": 3,
    "nameMaxLength": 24,
    "namePattern": "^[A-Za-z0-9 _'-]+$",
    "enforceUniqueNames": false,
    "createCooldownMs": 3000
  }
}
```

Defaults should be safe and backward compatible.

## Backward Compatibility Strategy

1. If no `characterSystem` settings exist, use defaults.
2. If profile has exactly one actor, optionally auto-spawn until UI rollout is complete (feature flag).
3. Preserve old behavior for offline mode unless explicitly enabled there.

Suggested flag:

- `characterSystem.enabled: boolean` (default `false` for staged rollout)

## Test Plan

### Unit/Integration

1. `selectCharacter` accepts owned actor id.
2. `selectCharacter` rejects non-owned actor id.
3. `createCharacter` rejects when max reached.
4. `createCharacter` rejects invalid name.
5. `createCharacter` succeeds and actor appears in subsequent list.
6. Disconnect between list and selection does not crash server.

### Manual QA

1. First login with no characters: creation flow works.
2. Returning player with 3 characters: selection list appears.
3. Attempt packet spoof (`actorId` from another profile): rejected.
4. Rapid create spam: rate limiter rejects excess requests.

## Rollout

1. Ship behind `characterSystem.enabled=false`.
2. Enable on staging.
3. Validate logs/metrics and persistence behavior.
4. Enable on production servers gradually.
5. Remove temporary compatibility branch after stable period.

## Risks and Mitigations

- Race conditions during async selection/create.
  - Mitigation: always re-check connection and ownership before final bind.

- Data inconsistency in legacy actors with missing metadata.
  - Mitigation: compute fallback values (`name="Unknown"`, `slot=actorId`).

- UI desync between packet and rendered state.
  - Mitigation: keep server authoritative; client re-requests list on mismatch.

## Definition of Done

1. A player can choose among multiple characters for one profile.
2. Unauthorized character selection is impossible server-side.
3. Creation limits and validation are enforced.
4. Flow is covered by automated tests and manual QA checklist.
5. Documentation and settings reference are updated.
