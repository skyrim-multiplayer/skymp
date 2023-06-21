# skymp5-functions-lib

A collection of function implementations for server-side Papyrus running in SkyMP servers. It's like SKSE for single-player Skyrim but uses TypeScript for registering natives instead of C++.

```typescript
mp.registerPapyrusFunction('global', 'Example', 'Hello', () => 'World');
```

```papyrus
Scriptname Example
String Function Hello() global native

; ...

Function Foo()
  String s = Example.Hello() ; "World"
EndFunction
```

## Motivation

As of January 2021, SkyMP had a powerful [API](https://pospelovlm.gitbook.io/skyrim-multiplayer-docs/docs_serverside_scripting_reference) that can be used to extend the multiplayer in different ways: adding custom synchronizations, graphical interfaces, or any other functionality.

Feedback on this feature was controversial. On the other hand, the API basically allowed to mod SkyMP in any manner. On the other hand, there were serious problems in real-world usage:

- Modmakers didn't want to deal with TypeScript.
- People loved calculating everything on the client creating tons of vulnerabilities.
- Performance problems.
- Bugs and code quality problems. Btw, SkyMP API itself is pretty strange.
- Every community server reimplements the same synchronizations and systems, non-compatible to other servers.
- Nothing forces internationalization. Lots of non-English string constants in the code.

SkyMP also had limited server-side Papyrus support, which allowed to run scripts from vanilla Skyrim and mods on the server in a safe and synchronized way. But there was a lack of Papyrus natives. SkyMP only had ~20 most used functions like `ObjectReference.AddItem` and `Utility.Wait`. So server-side Papyrus was more like a toy rather than a real scripting tool.

## Solution

We introduce `skymp5-functions-lib`, a collection of Papyrus functions that mimic vanilla/SKSE functions or being added especially for SkyMP (like `M.GetPlayersOnline`).

- Papyrus is simple for modmakers and generally simple.
- Papyrus doesn't have access to filesystem, eval, networking.
- Clientside scripting isn't exposed to Papyrus.
- Functions lib is covered with tests decreasing bugs.
- Standardized library of functions reduces incompatibilities and also prevents rewriting the same basic features for different game servers.

## Getting started

Installation

```
yarn add -D parcel-bundler
yarn
```

Building

```
yarn build
```

Watch for changes and build

```
yarn serve
```

Running tests

```
yarn test
```
