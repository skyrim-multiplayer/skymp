# Hot Reload

- Hot Reload for SkyrimPlatform plugins is supported. Changing the contents of `Data/Platform/Plugins` or `Data/Platform/PluginsDev` will reload all plugins without restarting the game.
- For using all these features, including Ctrl+S hot reload, take our example plugin as a base https://github.com/skyrim-multiplayer/skymp/tree/main/skyrim-platform/tools/plugin-example. Plugin example is also present in the archive uploaded to Nexus Mods (Data/Platform/plugin-example).
- When reloading plugins, the added event and hook handlers are removed, asynchronous operations are interrupted and all variables are reset, except for `storage` and its properties.

# Changing game console commands

- SkyrimPlatform allows you to change the implementation of any game console command, for such a modification you need to get the console command object by passing the command name to the `findConsoleCommand (commandName)` method, short or long.

  ```typescript
  let getAV = findConsoleCommand("GetActorValueInfo");
  let getAV = findConsoleCommand("GetAVInfo");
  ```

- Having received such an object, you can change the short (`shortName`) or long (`longName`) command name, as well as the number of accepted arguments (`numArgs`) and the function (`execute`) that will be executed when this console command is called via game console.

  ```typescript
  getAV.longName = 'printArg';
  getAV.shortName = '';
  getAV.execute = (refrId: number, arg: string) => {
    printConsole(arg);
    return false;
  };
  ```

- The return value of your new implementation indicates whether the original function of this command will be executed.
- The first argument is the FormId of the object on which the console command is called, or 0 if it is absent.
- The rest of the parameters will be the arguments with which the console command was called, of type `string` or `number`.
- Since game functions are not available in this context, you must register an `update` event handler with` once` if you want to call a game function when you invoke a console command:

```typescript
getAV.longName = 'ShowMessageBox';
getAV.shortName = '';

getAV.execute = (refrId: number, arg: string) => {
  once('update', () => {
    Debug.messageBox(arg);
  });
  return false;
};
```

# DumpFunctions

- SkyrimPlatform has built-in functionality that allows you to output information about game functions to the file `Data / Platform / Output / DumpFunctions.txt` (key combination 9 + O + L). The game pauses for a few seconds while DumpFunctions is running.

# Unhandled exceptions

- Unhandled JS exceptions will be logged to the console along with the call stack.
- Raw Promise rejections are also output to the console.
- Do not release plugins that have known bugs that are not handled. SkyrimPlatform performance is not guaranteed with unhandled exceptions.
