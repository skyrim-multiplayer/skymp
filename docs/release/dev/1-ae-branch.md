# AE branch description.

## List of changes
- CommonlibSSE updated to latest version.
- All raw offsets for hooks and game classes were replaced with Address Library IDs to support future game version changes.
- SKSE64 is no longer a dependency for skyrim platform. Everything that relied on skse was rewritten to use Commonlib classes\functions. Some game classes and offsets that are not currently supported by CommonlibSSE (main repository) are stored in `platform_se/game` folder.
- Project now complies with c++ 20 standard and compiles with VS 2022 without any issues.
- `skyrim_patform` and `skyrim_platform_entry` now use precompiled headers. Because of this most of external headers were stripped from skyrim platform sources and put into `PCH.h`.
- `skyrim_platform_entry` now relies on CommonlibSSE because of the new plugin initialization mechanism of skse runtime. NB: `MpClientPlugin.dll` still uses old initialization technique, and might not be initialized by skse runtime. No testing was conducted.
- Commonlib hooks introduced. Those can be used to hook into function calls or into class VTable with known offset IDs. Branch hooks are also supported, but require assembler code (or xbyak) which makes them non trivial.
- Frida hooks declaration and installation was refactored to have a similar structure to Commonlib hooks, which (*subjectively*) makes it cleaner and easier for developers to use than before.
- Logging introduced. Platform now uses skse runtime logging capabilities to provide users more feedback about its execution succession. Log file is located at `User\Documents\My Games\Skyrim Special Edition\SKSE`. Supports multiple log levels. Hopefully more codebase will be covered with logs with time.
- Runtime event sink handling introduced. Except for some custom events that fire from within the code, events now will not be processed if there are no plugins that are subscribed to them. In theory this should provide more execution performance since there are less calls that need to be processed.
- Platform now supports file stored user setting. Settings file is located at `Data\SKSE\Plugins\SkyrimPlatform.ini`.
- It is now possible to compile skyrim platform versions for Skyrim SE and Skyrim AE using appropriate CMake flags.
- Other minor code fixes and styling changes were made.

## Build requirements
- Building with both VS 2019 and VS 2022 is supported.
- Up-to-date Windows SDK is required. Version 10.0.22000 is confirmed to work well.
- NodeJS version 16.14 is required. Or you must manually update node-gyp to version 8.4.0+.
- If using system-wide vcpkg recommended version is `{ commit: c64c0fdac572ca43ea5ae018fc408ddced50d5b1 }` of `February 1, 2022` or higher.

## Known bugs & problems
- footstep event freezes the game at sink activation on Skyrim v1.6+.
- sceneActionEvent needs to be decompiled further to provide correct arguments.
