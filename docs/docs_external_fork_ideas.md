# External Fork Ideas

This document collects interesting ideas, features, and improvements found in public forks of the skymp repository. It is intended as a reference for future development inspiration and potential cherry-picks.

**Last scan:** 2026-05-03  
**Method:** Compared all forks against `skyrim-multiplayer/skymp:main` via `git ls-remote` and `git log --oneline --left-right`; 118 forks checked, ~25 had commits ahead of upstream.

---

## Top Cherry-Pick Candidates

### 1. HALFIN-Games/skymp-SkyV — Ed25519 Join Ticket Verification

**Commit:** `e42b0ddc`  
**Title:** `Server: verify website join ticket (Ed25519)`  
**Risk:** Medium (likely conflicts with current `login.ts`)

**What it does:**  
Adds cryptographic verification of join tickets issued by the web backend. A `JoinTicketVerifier` class validates Ed25519-signed JWTs (via `jose`), checking standard claims (`iss`, `aud`, `exp`, `iat`, `sub`, `jti`) plus custom claims like `whitelisted`, `queue_points`, `reserved_ok`, and `slots`. Configuration is done via environment variables.

**Files changed (5, +217/-3):**
- `skymp5-server/ts/systems/login.ts` — integration point
- `skymp5-server/ts/utils/joinTicket.ts` *(new)* — `JoinTicketVerifier` class
- `skymp5-server/ts/dev/verifyJoinTicket.ts` *(new)* — CLI dev tool for manual ticket testing
- `skymp5-server/join_ticket_ed25519_public.pem` *(new)* — example public key
- `skymp5-server/package.json` — added `jose` dependency

**Relevant env vars:**
```
JOIN_TICKET_ED25519_PUBLIC_KEY_PEM
JOIN_TICKET_ISSUER
JOIN_TICKET_AUDIENCE
```

**Other notable commits in this fork:**
| Commit | Title |
|--------|-------|
| `c8674675` | Multi-character slot system |
| `59e65269` | Multi-character slots follow-up |
| `9c4e6f95` | Voice chat adapter |
| `b0a5dd30` | Voice chat improvements |
| various | Ops documentation (VPS runbook, EC2 guide, tester kit) |

---

### 2. aw-c/skymp — Generic NAPI Serialization Archive

**Commit:** `6361d003`  
**Title:** `init new serialize`  
**Risk:** High (large Inventory refactor, C++20 required)

**What it does:**  
Introduces a `NapiOutputArchive` class as a type-safe, template-based serializer for converting C++ structs directly to NAPI/Node.js objects — without manual field-by-field `napi_value` construction. Uses C++20 concepts to dispatch on type:

```cpp
template<class T> concept IntegralConstant = ...;
template<class T> concept StringLike       = ...;
template<class T> concept Arithmetic       = ...;
template<class T> concept ContainerLike    = ...;
template<class T> concept NoneOfTheAbove   = ...;
```

`Inventory` is migrated as a proof-of-concept — it gains a generic `template<class Archive> void Serialize(Archive&)` method, removing duplicate JSON and NAPI conversion code.

**Files changed (4, +364/-193):**
- `serialization/include/archives/NapiOutputArchive.h` *(new)*
- `serialization/CMakeLists.txt`
- `skymp5-server/cpp/server_guest_lib/Inventory.cpp`
- `skymp5-server/cpp/server_guest_lib/Inventory.h`

> **Note:** `houndlord/skymp` contains a very similar serialization pattern plus additional changes in `ScampServer.cpp` and `index.ts` — worth comparing both.

---

### 3. q1000treadz/skymp — Client ActorValues Helper

**Commit:** `4601e331`  
**Title:** `feat(skymp5-client): send changed health percentage after animation`  
**Risk:** Low (small change, but requires manual port to current client structure)

**What it does:**  
Extracts actor value reading into a dedicated `getActorValues()` helper function in the client. The helper reads health, stamina, and magicka percentages, correctly returning `0` for health when the actor is dead.

**Files changed (2, +16/-10):**
- `skymp5-client/src/front/components/actorvalues.ts` *(new helper)*
- `skymp5-client/src/front/skympClient.ts`

> **⚠ Port required:** The old `src/front/` structure no longer exists. Manual port targets:  
> - `skymp5-client/src/sync/actorvalues.ts`  
> - `skymp5-client/src/services/services/skympClient.ts`

---

## Other Notable Forks

### piotroszko/skyworld
**Ahead by:** ~105 commits, 1955 files changed  
**Nature:** Separate product line / major fork

Large-scale restructure with its own project hierarchy (`projects/*`), custom property bindings, VoIP integration, and extended OnHit & Damage system with own documentation. Unlikely to be cherry-picked directly, but good for architectural inspiration (especially VoIP and the extended damage system).

---

### F02K/skymp
**Ahead by:** ~88 commits, 159 files changed  
**Nature:** Gamemode + tooling additions

Contains a Frostfall-style gamemode implementation and a build assistant tool. Useful if adding survival mechanics or improving the build/dev tooling.

---

### reveriadive/skymp
**Ahead by:** ~35 commits, 77 files changed  
**Nature:** Client-side gameplay patches

Patches for inventory handling, teleport validation, and movement validation on the client side. Worth reviewing when working on anti-cheat or client sync correctness.

---

### peterkmg/skymp
**Ahead by:** ~37 commits, 123 files changed  
**Nature:** Skyrim VR / CommonLibVR build support

Adds CMake and build configuration to support compiling against CommonLibVR (Skyrim VR). Not relevant unless VR support is a target.

---

## How to Reproduce the Scan

```bash
# List all forks via GitHub API (replace TOKEN)
curl -s -H "Authorization: token TOKEN" \
  "https://api.github.com/repos/skyrim-multiplayer/skymp/forks?per_page=100" \
  | jq '.[].full_name'

# Add a fork as a remote and compare
git remote add halfin https://github.com/HALFIN-Games/skymp-SkyV
git fetch halfin

# Commits in fork not in upstream
git log --oneline upstream/main..halfin/main

# Ahead/behind count
git rev-list --left-right --count upstream/main...halfin/main
```

---

*Forks excluded from this document: `RealHartlMax/skymp` (maintainer's own fork).*
