# CMake Workflow

Our build system is CMake-based. This document describes some caveats of our CMake code and guides you in making changes in CMake parts of the codebase.

When you switch between commits, you should run `cmake .. ` in the `build` directory. This action is called "CMake re-generation".

Only `Visual Studio 16 2019` generator is supported on Windows.

## Adding Source Files

* **Visual Studio**: `Add -> New Item -> /a meaningful source directory, not the build directory/`. No need to re-generate project files.
* **VS Code**: Add file normally, then re-generate project files. CMake extension for VS Code normally re-generates each time you press Ctrl+S in root `CMakeLists.txt`.

## Modifying CMakeLists

* When the content of CMakeLists has changed, you need to re-generate CMake.

* Call `apply_default_settings` for all added targets:
  ```cmake
  apply_default_settings(TARGETS skyrim_platform)
  ```

* When you add a new C/C++ target, that requires dependencies from vcpkg, you do not need to link them manually `target_link_libraries`, but simply add such target to the `VCPKG_DEPENDENT` list:
  ```cmake
  list(APPEND VCPKG_DEPENDENT TSConverter)
  ```

* Usually project's CMakeLists.txt has something like this:
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

## Troubleshooting

```
 CMake Error at vcpkg/scripts/buildsystems/vcpkg.cmake:857 (_find_package):
   Could not find a configuration file for package "directxtk" that is
   compatible with requested version "".
```
This error has been reported by VS Code user. Solution:
1. It seems that `amd64_x86` kit is selected. SkyMP doesn't support `x86` builds currently. Change the active kit to `amd64`.
   ![image](https://user-images.githubusercontent.com/37947786/125172169-cb8e4080-e1c0-11eb-8e72-b16b47908e39.png)
   ![image](https://user-images.githubusercontent.com/37947786/125172181-df39a700-e1c0-11eb-9d75-d576cf563c22.png)
2. Remove `build/CMakeCache.txt` and `build/CMakeFiles`
3. Re-generate project files.
