# Zone Handover and Instance Routing Roadmap

This document proposes a production-oriented handover workflow for location transitions (cities, interiors, dungeons) that happen behind Skyrim loading screens.

Goal:

- keep MMORPG social experience (players meet in the same places by default)
- improve server scalability and fault isolation
- prepare optional overflow sharding for crowded hotspots

## Design Principles

1. Shared by default

- The default behavior remains shared zones (same world/cell means same social space).
- Do not introduce per-player private instances as a default mode.

2. Technical separation for performance

- Zone simulation can run on separate workers/processes.
- This is a runtime partitioning strategy, not a gameplay isolation strategy.

3. Controlled overflow sharding

- Create additional shard copies only when a zone exceeds capacity.
- Keep party and group members in the same shard whenever possible.

## Why Loading Screen Is the Right Handover Point

A loading screen already represents a hard scene transition. It provides a safe and user-transparent window for:

- state snapshot
- ownership transfer
- target-zone initialization
- deterministic completion acknowledgement

This minimizes visual artifacts and race conditions during movement/combat.

## Proposed Handover Workflow

### Phase 1: Pre-Handover (source zone)

- Client requests transition to target location.
- Source worker places actor into transfer-lock state.
- Source worker captures an authoritative snapshot:
  - transform and worldOrCell
  - actor values and death/respawn state
  - inventory/equipment deltas
  - active effects and selected runtime flags
- Source worker publishes transfer token and payload to router/state store.

### Phase 2: Transfer (routing)

- Zone router selects target worker/shard based on:
  - target location key
  - shard occupancy and health
  - party affinity rules
- Target worker validates token and creates/loads actor runtime state.
- Source worker keeps actor reserved until target ACK is confirmed.

### Phase 3: Post-Handover (target zone)

- Target worker sends spawn/teleport data.
- Client exits loading screen only after ready ACK.
- Source worker finalizes cleanup after commit confirmation.

## Consistency and Safety Requirements

1. Idempotency

- Duplicate enter requests must not create duplicate actors.
- Transfer token must be single-use.

2. Atomic ownership switch

- Actor ownership is either source or target, never both.

3. Timeout and rollback

- If target fails to ACK, source restores actor and cancels transfer.

4. Ordering guarantees

- Drop stale movement/combat packets from old ownership epoch.

5. Observability

- Track handover latency, failures, retries, and rollback counters.

## Integration Points in Current SkyMP Architecture

Current implementation signals relevant seams:

- Single world container in runtime:
  - `skymp5-server/cpp/server_guest_lib/PartOne.h` (`WorldState worldState`)
- Main tick loop where world simulation runs:
  - `skymp5-server/cpp/server_guest_lib/PartOne.cpp` (`worldState.Tick()`)
- Existing location boundary key in protocol:
  - `skymp5-server/cpp/messages/TeleportMessage.h` (`worldOrCell`)
- Client-side world/cell mismatch handling:
  - `skymp5-client/src/sync/movementApply.ts`

These are good anchors for introducing a router/worker split while preserving gameplay behavior.

## Suggested Incremental Rollout

### Milestone 1: Protocol and transfer state

- Add transfer token and ownership epoch to movement/teleport path.
- Add transfer-lock state on source side.
- Add basic timeout + rollback behavior.

### Milestone 2: Router and worker abstraction

- Introduce zone router interface.
- Keep single-process implementation first (logical workers).
- Validate end-to-end handover invariants with tests.

### Milestone 3: Multi-worker deployment

- Run selected zones on separate processes.
- Add metrics dashboards and failure alarms.
- Validate reconnect behavior during in-flight handovers.

### Milestone 4: Optional overflow shards

- Enable adaptive shard creation for overcrowded locations.
- Enforce party affinity and social cohesion rules.
- Keep default path on shared shard for normal population.

## Non-Goals (for initial implementation)

- Per-player private instances as default gameplay model
- Cross-shard combat in first version
- Full distributed simulation rewrite before proving handover correctness

## Acceptance Criteria (MVP)

- Entering interiors/dungeons uses transfer workflow with deterministic completion.
- No duplicated actor ownership after transition.
- Failed transfer safely rolls back without player loss.
- Handover p95 latency remains acceptable under load (define threshold in ops runbook).
- Shared social behavior remains unchanged for normal population levels.
