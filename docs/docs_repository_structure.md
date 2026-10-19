# Repository Structure

Primary project languages are C++ and TypeScript.

Build system is CMake-based. If you work with the whole project, CMake would automatically invoke NPM commands for TypeScript subprojects to install dependencies, compile source files, etc.

## Special Folders

- `<repo_root>/build` - Hardcoded CMake build directory. Stores build artifacts. Obviously, not tracked by Git.

- `<repo_root>/cmake` - CMake scripts.

- `<repo_root>/overlay_ports` - Vcpkg overlay ports.

- `<repo_root>/overlay_triplets` - Vcpkg overlay triplets.

## Project Commons

Each project has its folder: `<repo_root>/<project_name>`

Regardless of language, we use kebab-case for folder names. It's a snake-case variant that uses a hyphen instead of an underscore. (i.e. `skyrim-platform` instead of `skyrim_platform` or `skyrimPlatform`).

Every project has `CMakeLists.txt`.

Project's `CMakeLists.txt` should define target with the same name as the project.

## C++ Projects

Use camel case for file names: `JsEngine.h`.

## TypeScript Projects

Use lower camel case for file names: `fooBar.ts`.

## Frontend structure (`skymp5-front`)

Main source code lives under `skymp5-front/src`.

- `src/features/adminDashboard` - Web admin dashboard pages, panels and settings forms
- `src/features/serverList` - launcher/server list UI
- `src/features/login` - frontend login flows
- `src/features/chat` - chat UI feature module
- `src/features/skillsMenu` - skills UI feature module
- `src/features/animList` - animation list UI feature module
- `src/features/testMenu` - development/testing UI module
- `src/locales` - i18n resources (`en`, `ru`, `de`, `es`, `fr`, `it`)

## Server TypeScript structure (`skymp5-server/ts`)

- `index.ts` - server entrypoint bootstrap
- `ui.ts` - HTTP routes (`/api/admin/*`, metrics, admin auth/session flows)
- `settings.ts` - `server-settings.json` loading, validation and additional settings merge
- `manifestGen.ts` - resource/load-order manifest generation helpers
- `scampNative.ts` - native bridge bindings used by server runtime
- `systems/` - gameplay/runtime systems
- `examples/` - sample scripts (for example healer/respawn flows)
- `shims/` - compatibility shims used during bundling/runtime
