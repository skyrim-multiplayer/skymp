# Server Update Playbook (No Progress Loss)

This guide is for server operators who want to update without reinstalling from scratch and without losing player progress.

## Update Goal

- Replace runtime binaries/scripts.
- Keep persistent state (player progress, moderation state, configuration, data files).
- Keep rollback path ready.

## What Must Persist

Keep these outside of "throwaway release folders" and include them in backups.

- `server-settings.json`
- `dataDir` (default `./data`), including:
  - `changeForms`
  - `admin-auth.json`
  - `admin-bans.json`
  - `admin-mutes.json`
  - `admin-history.json`
  - `admin-player-stats.json`
  - your mods/scripts/UI assets
- your database files/dumps (if not purely in-memory)

References:

- [Server Data Directory](docs_server_data_directory.md)
- [Server Configuration Reference](docs_server_configuration_reference.md)
- [Running A Server](docs_running_a_server.md)

## Release Classification Policy

Use this to communicate operator risk clearly.

- `patch` (`fix`) - no migration expected, low risk
- `minor` (`feat`) - new functionality, usually backward-compatible
- `major` (`major`, `breaking`, `!`, or `BREAKING CHANGE`) - migration required or behavior compatibility risk

## Commit Message Convention (Release Trigger)

Automatic server release is created only when at least one commit since the last release
matches one of the accepted triggers:

- `fix(...)` -> patch release
- `feat(...)` -> minor release
- `major(...)`, `breaking(...)`, `type!:` or `BREAKING CHANGE` -> major release

If no commit matches these triggers, the workflow skips the release.

Changelog behavior:

- Release notes include all commits since the last release tag.
- Commits are grouped into Major, Features, Fixes and Improvements, and Other Changes.
- This means commits without trigger keywords are still included in changelog once a triggered release happens.

Examples:

- `fix(auth): handle expired admin session token`
- `fix: prevent crash on empty player list`
- `feat(admin): add update banner in dashboard`
- `feat: support separate release metadata file`
- `major(server): switch default data layout`
- `feat(api)!: remove legacy endpoint format`

## Operator Procedure

1. Read release notes and classify risk (`patch`/`minor`/`major`).
2. Announce maintenance window to players.
3. Take backups (required):
   - `server-settings.json`
   - full `dataDir`
   - database snapshot/dump
4. Stop the server cleanly.
5. Deploy new server files.
6. Start server using existing persistent config/data.
7. Smoke test:
   - login works
   - existing player state is intact
   - admin dashboard works
   - moderation state is intact
8. If failed, rollback immediately (see below).

## Downloaded Build Update Commands

If the operator updates from downloaded release archives instead of CI deploy, use the helper scripts from the server package root.

Windows:

```powershell
.\update_server.ps1 -PackagePath C:\path\to\running_server_files_windows_server_dist.zip
```

Linux:

```sh
./update_server.sh --package /path/to/running_server_files_linux_server_dist.tar.gz
```

Optional automatic restart after file copy:

- Windows: add `-StartAfter`
- Linux: add `--start-after`

When auto-restart is used, both update scripts start the packaged launcher (`launch_server.bat` / `launch_server.sh`) instead of invoking `node` directly. This keeps launcher-side safety checks active, including stale-process cleanup on Windows and runtime dependency bootstrap on first start.

Both scripts preserve:

- `data/`
- `server-settings.json`
- `server-settings-dump.json`
- `server-settings-merged.json`

Both scripts also create a timestamped backup under `./backups/` before replacing runtime files.

For downloaded builds, `node_modules` is allowed to be absent in the archive. The packaged launchers install runtime npm dependencies on first start when needed.

## Rollback Procedure

1. Stop the updated server.
2. Re-point service to previous release folder (or restore previous package).
3. Restore backups if persistent files were changed.
4. Start server.
5. Publish rollback status to operators/players.

## Recommended Deployment Layout

Use a stable structure where release code and persistent data are clearly separated.

Example:

```text
/opt/skymp/
  releases/
    v1.0.0/
    v1.0.1/
  current -> /opt/skymp/releases/v1.0.1
  persistent/
    server-settings.json
    data/
```

Then configure `dataDir` to point to the persistent location.

## Communication Checklist (Per Release)

- What changed (short bullet list)
- Risk level (`patch`/`minor`/`major`)
- Required operator actions (if any)
- Backup reminder (always)
- Planned maintenance window
- Rollback statement
