# Admin Discord OAuth

## Purpose

This document describes the Discord OAuth login for the SkyMP admin panel and the planned production model where server owners do not need to manually set tokens or environment variables.

## Current behavior

The admin login page has two options:

- Username/password login (existing flow)
- Discord OAuth login button (new flow)

Discord OAuth callback maps Discord User ID to an admin user entry.
If there is no matching admin user with the same `discordId`, login is denied.

## Data model

Admin users are stored in `server-settings.json` via:

- `adminUiUsers` (profile map with role and optional `discordId`)
- `adminUiRoles` (role map)
- `adminUiMasterUser` (primary master admin user)

Master admin comes from first setup (or legacy migration) and is persisted automatically.

## Planned integration model (no manual token setup)

Target state: users should not have to add custom env vars or secrets by hand.

Planned architecture:

1. SkyMP ships a built-in admin auth provider module.
2. OAuth app credentials are provisioned and managed centrally by platform tooling.
3. Server receives a signed trust payload after successful OAuth, instead of local secret handling.
4. Admin mapping stays local (`discordId` -> admin profile), while OAuth secrets remain managed by platform.
5. Rotation, revocation, and expiry are handled automatically by platform services.

## Why this model

- No manual secret handling for server operators
- Lower setup friction
- Fewer misconfigurations
- Centralized credential rotation and audit trail

## Security requirements

Even in the fully integrated model, the following remain mandatory:

- CSRF state verification in OAuth start/callback
- Strict callback URL allow-list
- Short-lived session cookies
- Role/permission enforcement after login
- Deny login for unmapped Discord IDs

## Migration path

1. Keep username/password login available as fallback.
2. Add Discord ID to each admin account in Admins topbar.
3. Enable integrated Discord OAuth for the instance.
4. Validate that mapped Discord accounts can sign in.
5. Optionally disable password login for non-master admins later.

## Notes for implementers

- Discord ID must be treated as immutable user identifier.
- Username shown in UI is display metadata, not auth identity.
- Master admin must always stay visible and non-deletable in Admins list.
