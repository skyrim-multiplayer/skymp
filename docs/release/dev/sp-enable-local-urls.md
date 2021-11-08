## Enable local URLs in browser

Now SkyrimPlatform loads `Data/Platform/UI/index.html` if the file exists. It is also possible to load local URLs in runtime.

```ts
// We were able to load only remote URLs. This feature was used in multiplayer but was completely useless for single-player mods.
browser.loadUrl("https://skymp.io");

// Now we can load paths from our local filesystem.
browser.loadUrl("file:///Data/Platform/UI/index.html");
```
