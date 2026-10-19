# Admin Console Commands

The server admin console now supports a small command dispatcher with explicit permissions and a legacy JavaScript fallback.

## Supported Commands

- `help [command]` - show the command list or a short description.
- `players` - list online players with `userId`, `actorId`, and actor name.
- `kick <userId> [reason...]` - kick a player.
- `kick-all [reason...]` - kick all online players.
- `ban <userId> [durationMinutes] [reason...]` - ban a player.
- `unban <userId>` - remove a ban.
- `mute <userId> [durationMinutes] [reason...]` - mute a player.
- `unmute <userId>` - remove a mute.
- `warn <userId> <message...>` - send a warning message.
- `message <userId> <message...>` - send a private message.
- `announce <message...>` - show a visual announcement toast to all online players.
- `js <chakraJavaScript...>` - execute raw Chakra JavaScript, for admins only.

## Aliases

A few shorthand aliases are accepted for convenience:

- `commands` -> `help`
- `?` -> `help`
- `listplayers` -> `players`
- `lsplayers` -> `players`
- `say` -> `announce`
- `msg` -> `message`
- `pm` -> `message`
- `kickall` -> `kick-all`
- `eval` -> `js`

## Permissions

Command access is checked per action through admin capabilities:

- `canViewLogs` for `help` and `players`
- `canKick` for `kick` and `kick-all`
- `canBan` for `ban`
- `canUnban` for `unban`
- `canMute` for `mute`
- `canUnmute` for `unmute`
- `canWarn` for `warn`
- `canMessage` for `message` and `announce`
- `canConsole` for `js`

If a command is not recognized and the user still has `canConsole`, the server keeps the old JavaScript execution fallback for compatibility.

`announce` is rendered as a top-screen HUD banner with a fade-in / fade-out animation instead of a chat message.

## Examples

```text
help
players
kick 12 "spamming chat"
ban 12 1440 "griefing"
mute 18 30 "temporary mute for moderation review"
announce "Server restart in 5 minutes"
js console.log('admin test')
```

Typical usage patterns:

- Use `help` first if you are not sure which commands your role can access.
- Use `players` to look up the `userId` before running moderation commands.
- Put reasons in quotes when they contain spaces.
- Use `js` only for troubleshooting or legacy workflows, not as the main admin path.

## Notes

- The dispatcher trims and tokenizes quoted arguments, so messages with spaces can be passed safely.
- Moderation commands also write to admin history and server logs where applicable.
- The implementation lives in `skymp5-server/ts/ui.ts`.
