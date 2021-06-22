### После изменения версии vcpkg необходимо

- Удалить следующие файлы и директории:

```
skyrim-platform\build\CMakeFiles
skyrim-platform\build\CMakeCache.txt
skyrim-platform\build\_platform_se_\CMakeFiles
skyrim-platform\build\_platform_se_\CMakeCache.txt
```

- Перегенерировать проект (`cd C:/Projects/skyrim-platform/build && cmake -DCEF_DIR=C:/Projects/cef64 -DSKYRIM_DIR="C:/Program Files (x86)/Steam/steamapps/common/Skyrim Special Edition" ..`)

### Building package directxtk:x64-windows-static failed with: BUILD_FAILED

- Откройте `C:\Program Files (x86)\Windows Kits\10\include\10.0.17763.0\winrt\wrl\module.h` на строке `1427`

- Удалите или закомментите эти строчки:

```c++
        Details::CheckForDuplicateEntries((GetFirstEntryPointer() + 1), GetMidEntryPointer(),
            [](const Details::CreatorMap* entry, const Details::CreatorMap* entry2) -> void {
                __WRL_ASSERT__(entry->activationId.clsid != entry2->activationId.clsid && "Duplicate CLSID!");
            }
        );

        Details::CheckForDuplicateEntries((GetMidEntryPointer() + 1), GetLastEntryPointer(),
            [](const Details::CreatorMap* entry, const Details::CreatorMap* entry2) -> void {
                __WRL_ASSERT__(::wcscmp((entry->activationId.getRuntimeName)(), (entry2->activationId.getRuntimeName)()) != 0 && "Duplicate runtime class name!");
            }
        );
```
