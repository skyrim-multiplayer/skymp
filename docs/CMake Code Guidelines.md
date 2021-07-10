# CMake Code Guidelines

Our build system is CMake-based. This document describes some caveats of our CMake code and guides you in making changes in CMake parts of the codebase.

## CMake Workflow

### Changing Project Revision

When you switch between commits, you should run `cmake .. ` in the `build` directory. This action is called "CMake re-generation".

### Adding Source Files

* **Visual Studio**: `Add -> New Item -> /a meaningful source directory, not the build directory/`. No need to re-generate project files.
* **VS Code**: Add file normally, then re-generate project files. CMake extension for VS Code normally re-generates each time you press Ctrl+S in root `CMakeLists.txt`.

## Modifying CMakeLists

When the content of CMakeLists has changed, you need to re-generate CMake.

### Adding Targets

Call `apply_default_settings` for all added targets:
```cmake
apply_default_settings(TARGETS skyrim_platform)
```

When you add a new C/C++ target, that requires dependencies from vcpkg, you do not need to link them manually `target_link_libraries`, but simply add such target to the `VCPKG_DEPENDENT` list:

```cmake
list(APPEND VCPKG_DEPENDENT TSConverter)
```

Usually project's CMakeLists.txt has something like this:
```cmake
foreach(target ${VCPKG_DEPENDENT})
  # link everything
endforeach()
```

## CMake Errors

When generating project files with CMake, errors are dumped into the console. If generation fails, then you see these lines in your terminal:
```
- Configuring incomplete, errors occured
```

It is necessary to look above and find `CMake Error at...`. There would be a path and a line number. By the way, VS Code is able to highlight this.
