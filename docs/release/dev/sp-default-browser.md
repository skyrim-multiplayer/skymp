## Support opening URLs in the system default browser

Skyrim Platform already had in-game browser support for creating user interfaces powered by Web technologies.

In this update, we introduce the ability to open URLs in the user's default browser: Chrome, Firefox, Edge, Opera, etc.

Now you are able to create a link or a button or a hotkey or any other trigger that will direct your mod's users to your Nexus, GitHub, or Patreon page. Or even to your Website.

```typescript
win32.loadUrl("https://github.com/skyrim-multiplayer/skymp");
```

Currently, only URLs prefixed with `https://` are allowed.
