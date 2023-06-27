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
