## Add WebPack support

SkyrimPlatform is now able to load plugins built by WebPack. Raw TSC output is still supported and the example plugin didn't migrate. See [skymp5-client](https://github.com/skyrim-multiplayer/skymp/tree/479562345a1f6df4af42217936ccb3e2d3819f78/skymp5-client) for example of use.

Thanks to WebPack support we are now able to use packages from NPM in our SkyrimPlatform plugins. For example, [RxJS](https://rxjs.dev/guide/overview) is proven to work.

Note: packages using timer functions (`setTimeout`, `setInterval`, etc) will not work until we implement these APIs. This also applies to other browser-specific or node-specific APIs.
