# Creating your first Skyrim Platform plugin!

- [Install Skyrim Platform](#install-skyrim-platform)
- [Install Node.js](#install-nodejs)
- [Install VSCode](#install-vscode)
- [Copy Example Plugin](#copy-example-plugin)
- [Open Example Plugin](#open-example-plugin)
- [Configure Skyrim Path](#configure-skyrim-path)
- [Configure Plugin Name](#configure-plugin-name)
- [Compile Plugin](#compile-plugin)
- [View Plugin Running in Skyrim](#view-plugin-running-in-skyrim)
- [View the main plugin file](#view-the-main-plugin-file)
- [Make your own changes to the plugin](#make-your-own-changes-to-the-plugin)
- [Importing third-party libraries](#importing-third-party-libraries)
- [Distributing your Skyrim Platform plugin](#distributing-your-skyrim-platform-plugin)
- [Read the Documentation](#read-the-documentation)
- [Join the Discord](#join-the-discord)
- [LICENSE](#license)

## Install Skyrim Platform

First off, you need to install Skyrim Platform (_using your favorite mod manager_)

- https://www.nexusmods.com/skyrimspecialedition/mods/54909

## Install Node.js

You will also need Node.js:

- https://nodejs.org

> You can download either the 'LTS' or the 'Current' version

## Install VSCode

It is highly recommended to download a text editor with good support for TypeScript.

Unless you prefer another editor, you should install [VSCode](https://code.visualstudio.com/).

> The example plugin has build-in support for VSCode

## Copy Example Plugin

You can find the `plugin-example` folder inside the downloaded Skyrim Platform mod.

If you installed Skyrim Platform using a mod manager, find and click on the Skyrim Platform mod.

Next, open the mod in the file system:

- Vortex: `Open in File Manager`
- Mod Organizer 2: `Open in Explorer`

You will see a `Platform` folder with a `plugin-example` subdirectory.

Copy the `plugin-example` subdirectory to someplace on your PC _outside of the Skyrim folder_.

> Unlike traditional Skyrim mods, you do not need to put your Skyrim Platform plugins into your MO2 or Vortex mods folder _unless_ your mod includes game assets, e.g. textures and meshes
>
> It is recommended that you _not_ put your plugin folder anywhere inside of a MO2 mod folder or inside of your Skyrim folder.

## Open Example Plugin

If you are using VS Code, open the example plugin by double-clicking on the `example-plugin.code-workspace` file.

Otherwise, open the plugin folder in your favorite text editor or IDE.

## Configure Skyrim Path

In the file explorer, open `skyrim.json` and edit the path so that it directs to your Skyrim folder:

```js
{
    "skyrimFolder": "C:/Program Files (x86)/Steam/steamapps/common/Skyrim Special Edition"
}
```

> If you prefer, you can set the `SKYRIMPATH` environment variable to this path.  
> This can be convenient when working with multiple projects or collaborators who have Skyrim folders in different locations.

## Configure Plugin Name

All plugins must have unique names.

Configure the name of your plugin by opening the `package.json` file and changing the `"name"` field.

For example:
```json
{
  "name": "my-cool-plugin",
  "version": "1.0.0",
```

> Please do not use spaces. Use only lowercase letters and dashes.

## Compile Plugin 

Now you are ready to compile and run your first Skyrim Platform plugin!

In the file menu, select `Terminal` > `Run Build Task...` (_or press Ctrl+Shift+B_)

This should automatically open a terminal which will:
- Install all plugin dependencies from npm
- Compile your plugin
- Bundle your plugin for distribution
- Copy your plugin to the Skyrim Platform plugins directory
- Watch for file changes (_which will automatically trigger these steps again_)

You can run this manually in a terminal via: `npm run dev`

You can manually confirm that your plugin file was successfully created and copied into the correct folder by viewing the `[Skyrim Folder]\Data\Platform\PluginsDev` directory. It should include a JavaScript file named `[your plugin name].js`

## View Plugin Running in Skyrim

Now run Skyrim. Be sure that you have installed Skyrim Platform.

When you reach the Main Menu, press ~ to open the Skyrim Console.

If Skyrim Platform was successfully installed, you should see the text:

```
Hello SE
```

If your plugin is running successfully, you should see:

```
[Script] Hello! You can view this in the Skyrim ~ console on the Main Menu when the game runs
```

If you would like to go into the game to quickly test your plugin, type the following in the ~ Skyrim console and run by pressing [Enter]:

```
coc riverwood
```

Once the game loads into Riverwood, you should see a dialog box popup with the message:

```
Hello! This will appear when a new game is started or an existing game is loaded
```

That's it! Now it's time to make changes to the plugin.

**You can Alt-Tab out of the game now to make changes and they will immediately trigger in your game!**

## View the main plugin file

In the example projects, the main plugin file can be found in the `src/` folder:

### `src/index.ts`
```ts
import { Debug, once, printConsole } from 'skyrimPlatform'

once('tick', () => {
    printConsole('Hello! You can view this in the Skyrim ~ console on the Main Menu when the game runs')
})

once('update', () => {
    Debug.messageBox('Hello! This will appear when a new game is started or an existing game is loaded')
})
```

## Make your own changes to the plugin

Edit the above code with your own text. For example:

```ts
import { Debug, once, printConsole } from 'skyrimPlatform'

once('update', () => {
    Debug.messageBox('Hey look! I changed this! Yay!')
})
```

If you still have the development process running (via `Run Build Task...` or `npm run dev`), the changes should automatically recompile and you should be able to go into the game immediately to view the changes!

When you Alt-Tab back into the game, you should get the popup immediately!

If you do not, make sure that your terminal is still running and that it does not show any errors.

## Importing third-party libraries

Any libraries you install as dependencies will be bundled into your plugin via `webpack`.

These may be other libraries you create yourself or other people's libraries.

A very common use-case is importing common Skyrim utility libraries such as [`PapyrusUtil`][PapyrusUtil] or [`JContainers`][JContainers].

If you need to use `PapyrusUtil` you can open up a terminal and run `npm i @skyrim-platform/papyrus-util` to install the `PapyrusUtil` TypeScript library for calling native `PapyrusUtil` functions from TypeScript.

Next, try updating your `src/index.ts` to be the following:

```ts
// Import the utility function from PapyrusUtil which can list folders in a file system path
import { FoldersInFolder } from '@skyrim-platform/papyrus-util/MiscUtil'
import { once, Debug } from '@skyrim-platform/skyrim-platform'

// When 'update' runs, show a messagebox listing all of the top-level folder names inside of the 'Data' folder
once('update', () => {
    const foldersInData = FoldersInFolder('.')
    if (foldersInData)
        Debug.messageBox(`Folders in Data directory: ${foldersInData.join("\n")}`)
})
```

This is a simple demonstration of how to use one function from one common Skyrim library.

For more documentation on how to use a third-party library, read the documentation on its npm page or GitHub repository.

> Third-party library documentation is not provided in this README

## Distributing your Skyrim Platform plugin

Once you are ready, you can package your Skyrim Platform plugin into a distributable .zip file which can be shared on mod sites, e.g. NexusMods.

To package your Skyrim Platform plugin, run:

```
npm run zip
```

This will create a file in your plugin folder named `[your plugin name from package.json]-[your plugin version from package.json].zip`

If you would like to include assets such as textures or meshes etc, you can manually archive your plugin. When doing so, your archive should contain a `Platform` folder with a `Plugins` subdirectory containing your bundled JavaScript plugin distributable file.

Structure of the zip archive for distribution:

#### `myplugin-1.0.0.zip`

```
Platform\
  Plugins\
    plugin-file-1.0.0.js    
```

> Note: do not try to package your .ts TypeScript files -or- the .js JavaScript files in the dist/ folder. Instead, use the .js file in the build/ folder which is bundled for usage with Skyrim Platform

## Read the Documentation

You are now ready to get started!

You can find documentation for Skyrim Platform at:

- https://github.com/skyrim-multiplayer/skymp/blob/main/docs/docs_skyrim_platform.md

## Join the Discord

For further support using Skyrim Platform, join the Discord server https://discord.gg/P8yg7YKY2e

## LICENSE

Your plugin is your own software and can be licensed however you like!

Skyrim Platform is distributed under the [GNU General Public License v3.0][GPLv3].

[PapyrusUtil]: https://www.nexusmods.com/skyrimspecialedition/mods/13048
[JContainers]: https://www.nexusmods.com/skyrimspecialedition/mods/16495
[GPLv3]: https://www.gnu.org/licenses/gpl-3.0.en.html
