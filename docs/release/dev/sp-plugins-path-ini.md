## Add `PluginFolders` INI Setting

Now you can customize folders to load SkyrimPlatform plugins from.

### New SkyrimPlatform.ini settings

List of plugin folders to load plugins from.

```
[Main]

PluginFolders = your_path_1;your_path_2
```

Whitespace characters allowed:

```
[Main]

PluginFolders = "your path with whitespace"
```

### Removed the hardcoded SkyMP path

The previously hardcoded path `C:/projects/skymp/build/dist/client/Data/Platform/Plugins` has been removed. This path was used to override standard paths if the folder existed, primarily for SkyMP development purposes.
