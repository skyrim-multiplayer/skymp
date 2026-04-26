# skymp5-chat

Minimal SkyMP client UI replacement for the legacy `skymp5-front` bundle.
It keeps the browser contracts that the client needs for login and chat:

- `window.skyrimPlatform.sendMessage(...)`
- `window.skyrimPlatform.widgets.set([...])`
- `window.mp.send(type, data)`

It builds the same client-facing file shape:

- `index.html`
- `build.js`
- `style.css`

The UI renders generic `form` widgets from the client auth service, so the
login, failure, Discord, back, and play screens can be shown before the player
enters the world.

It also exposes `window.skympChat.addMessage(rawMessage)` for server-rendered
chat messages and sends chat or command input through:

```js
window.mp.send('cef::chat:send', text)
```

Press `Enter` to open the chat input, `/` to open it pre-filled for commands,
and `Escape` to close it.

Submitted text is echoed locally right away. Normal chat echoes as `[You]` until
the server sends the real styled message back; slash input is kept as
`[Command]` so command usage is visible in the log. The client keeps a visible
session history in `sessionStorage` and asks the server to replay its in-memory
per-player history with `__reload__` after loading.

## Scripts

```sh
npm run build
npm run deploy:ui
npm run check
```

`deploy:ui` copies the built files to `../build/dist/client/Data/Platform/UI`.
You can pass a custom target folder as the first argument.
