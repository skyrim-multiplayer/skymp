# Skyrim Platform

Skyrim Platform is a modding tool for Skyrim allowing writing scripts with JavaScript/TypeScript.

One of the mods built on Skyrim Platform is skymp client.\
Yes, client of Skyrim Multiplayer is technically a mod for Skyrim Special Edition implemented using Skyrim Platform.

Here you will find documentaiton aimed for you to create your own mods using Skyrim Platform.

The documentation is versioned up-to-date with SkyrimPlatform itself. If you read this page on GitHub, it's probably docs on latest SP revision. We release SkyrimPlatform updates from time to time. See [here](https://github.com/skyrim-multiplayer/skymp/tree/main/docs/release/dev) what is included in the upcoming SP update.

## Versioning

SP follows [Semantic Versioning](https://semver.org/) for JavaScript, but not for TypeScript.

It means that non-major updates won't break compiled TypeScript plugins or plugins written in JavaScript. If they do, treat it as an SP bug then.

On the other hand, updating `skyrimPlatform.ts` may break the compilation of your TypeScript plugins. You likely will be able to easily fix these problems, or ask for help in our Discord server, or just use `skyrimPlatform.ts` from one of the previous versions.

## Table of contents

- Skyrim integration
  - [Papyrus objects][Papyrus]
  - [Native functions][Native]
  - [Hooks (events)][Events]
- Content added by Skyrim Platform
  - [Exclusive features][Features]
  - [New methods and properties][NewMethods]
  - [New types][NewTypes]
  - [New events][NewEvents]
  - [Browser / UI][Browser]
  - [HTTP][]
  - [Win32][Win32]
- [Cook Book][Cookbook]

[Browser]: skyrim_platform/browser.md
[Cookbook]: skyrim_platform/cookbook.md
[Events]: skyrim_platform/events.md
[Features]: skyrim_platform/features.md
[HTTP]: skyrim_platform/http.md
[Native]: skyrim_platform/native.md
[NewEvents]: skyrim_platform/new_events.md
[NewMethods]: skyrim_platform/new_methods.md
[NewTypes]: skyrim_platform/new_types.md
[Papyrus]: skyrim_platform/papyrus.md
[Win32]: skyrim_platform/win32.md
