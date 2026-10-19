# Server Operator Announcement Template

Use this template for Discord, forum posts, or GitHub release notes aimed at server operators.

## Subject

`[SkyMP] Server update {{VERSION}} ({{RISK_LEVEL}})`

## Message

Hello server operators,

we published server update **{{VERSION}}**.

### 1) Risk level

- Level: **{{patch|minor|major}}**
- Reason: {{one-line reason}}

### 2) What changed

- {{change 1}}
- {{change 2}}
- {{change 3}}

### 3) Required actions

- {{none OR explicit migration step}}
- {{config change if needed}}

### 4) Before updating (required)

- backup `server-settings.json`
- backup `dataDir` (`changeForms`, admin state files, scripts/mod assets)
- backup database/dump (if used)

### 5) Maintenance window

- Start: {{date/time + timezone}}
- Expected downtime: {{minutes}}

### 6) Rollback policy

If problems appear, revert to previous server package and restore backups.

### 7) Links

- Release: {{release URL}}
- Changelog: {{changelog URL}}
- Update guide: `docs/docs_server_update_playbook.md`

Thanks for running SkyMP servers.
