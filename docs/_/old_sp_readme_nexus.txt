Introduction

Skyrim modding was always about Papyrus and C++.

Papyrus is a "native" language for Bethesda's games. It was designed to extend game mechanics, run quests, and so on. But there are lots of limitations since only features that were really required to release Skyrim are present. Thanks to Papyrus we already can customize the game in various ways. However this language isn't super-modern, it doesn't support many features that other languages do support out of the box and it is really laggy.

On the other side, we have a lot of C++ modding stuff. C++ is the language the game industry bases on. It's enough high-level and also gives us the ability to go under the game's hood and do some reverse-engineering stuff. As a programming language, C++ is pretty hard to learn. Some people say C++ is one of the most overcomplicated and bloated-up technologies to work with. The entry threshold is pretty high here, especially for Skyrim modding: it's very easy to make a mistake, which would be super-hard to debug and fix; there is no one common SDK to develop mods: someone uses CommonLibSSE, someone, not; there is no simple way to use script functions from C++ code; the process of development is slow due to recompilation and the long game restarts. We love C++ but let's say it doesn't cover all modding needs.

SkyrimPlatform Is Here

Don't tell us this name is strange. SkyrimPlatform is a scripting platform for Skyrim SE created to neutralize the disadvantages of Papyrus/C++ modding as much as possible. Here I need to point our background. We in SkyMP need one really complex Skyrim mod - the client that allows us to play online. There are tons of features that need to be implemented in this "mod", but technically it's still a single-player modification. So. We created SkyrimPlatform as an instrument to mod vanilla Skyrim since we want to make a multiplayer modification which is basically still a mod for vanilla Skyrim.

Why do I tell you all these things here? To explain the context of this development. Keep in mind that many SkyrimPlatform design decisions were made according to our multiplayer development needs. But SkyMP is still a work in progress and I want to say one thing.

We develop SkyMP in the hope to be useful today, not after the date of the full SkyMP release.

That's why we release SkyrimPlatform as a powerful tool used as SDK for Skyrim in our team. Enjoy.

How does Skyrim Platform solve mod maker problems?

Hot reload

One of the principles of Extreme Programming is Fine scale feedback. Let's simplify and say that XP proposes unit testing apps to ensure that everything works as soon as the code piece is written. But we all know that we can not reliably unit test Skyrim mods. How do we get feedback fast? Without SkyrimPlatform we have to restart the game again and again: close Skyrim SE, rebuild the solution, copy files if your build system doesn't do it, restart the game, load saves, etc.

Here SkyrimPlatform introduces its killer feature: Hot reload of plugins. You just Ctrl+S in your favorite text editor, Alt+Tab to the game and see differences.

Modding Skyrim may be very frustrating: you change the code, restart everything, see that it still doesn't work, and repeat. SkyrimPlatform reduces this cycle. If you even don't know how the feature should work, you can try different combinations of code incredibly fast.

If you need a debug message, you can add it to your code and see the result without restarting anything and restoring the game state.

It's super-power allowing mod developers to catch bugs faster.

Maybe you guess why do we need SkyrimPaltform to work this way. To develop synchronizations for SkyMP. But it would also help in the development of another complex mod.

Global Scripts

Let's go back and speak about Papyrus. Papyrus scripts are attached to game entities: actors, object references, quests. In most cases, you need to attach your scripts to objects via Creation Kit. SkyrimPlatform plugins are independent of game entities: you don't have to attach them to one. It's more like an SKSE plugin rather than a Papyrus script. It's global, executing when the game executes.


TypeScript

One of the cornerstones of SkyrimPlatform is that it uses TypeScript for plugins. Gods, why in the world where the common scripting language for games is Lua? Agree. Lua is a super-thing, it shows itself as a powerful and easy to embed scripting language. LuaJIT is also pretty fast. But there also are downsides:

You can not see errors in compile time. There is just no compilation step. In many cases, you would be noticed that something is broken only when code execution reaches the incorrect place of code.
Lua's standard library is pretty pure.
Callback-based asynchronous operations.

TypeScript works better here:

Your favorite text editor would be able to show you most of your mistakes as soon as they are made. For example, with recommended TS settings you will be noticed if you try to call a method on the nullable Actor variable that you didn't check. Btw, also compare this behavior with Papyrus that just does nothing in this case.
The power of ECMAScript standard library: string, JSON, etc.
Promises, async/await. Your code isn't going into Callback Hell when you do `Utility.Wait`.

Stable Sandbox

SkyrimPlatform protects you from null pointers, multithreading issues, etc.

Replace Animations On The Fly

SkyrimPlatform contains an animation event hook that is exposed to TypeScript API. You can use it to replace or cancel animations when they happen. For example, we can prevent an actor to get up after ragdoll animation.

Create GUI Widgets With HTML/CSS/JS

SkyrimPlatform comes with Chromium Embedded Framework which allows us to use standard HTML5/CSS3/JavaScript for the UI. You can call loadUrl from your plugins.

Performance

You can execute your code on every game update, normally 60 times a second. Calling thousands of scripting functions doesn't kill FPS on our machines. But we will keep working to improve performance.

SKSE Compatibility

SkyrimPlatform is technically an SKSE plugin written in C++. When writing scripts, you can use standard SKSE functions and functions from SKSE plugins.

ES6 Modules

SkyrimPlatform supports imports/exports thanks to its own module loader which mimics systemjs.

FAQ

Q: How do I distribute my plugins?
A: SkyrimPlatform plugins are compiled into JS files (see tsconfig.json in plugin example). So you need to distribute your-plugin-name.js. Users will need to put it into Data/Platform/Plugins.

Q: SKSE plugins typically have ini configuration. How can users configure my SkyrimPlatform plugin?
A: There is a settings API (see docs). Settings are also stored Data/Platform/Plugins. Settings file suffix is "-settings.txt". Your settings file would probably be called your-plugin-name-settings.txt or something like this.

Q: How do I create a plugin?
A: See Data/Platform/plugin-example in the archive. It's an example plugin. This folder includes README.md describing how to use our plugin example.

Q: How do I get hot reload to work?
A: If you followed instructions from Data/Platform/plugin-example/README.md, just Ctrl+S in your favorite code editor.