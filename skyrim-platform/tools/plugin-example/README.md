# skyrimplatform-plugin-example

## Setting Up


1. Grab the latest version of the Skyrim Platform from https://www.nexusmods.com/skyrimspecialedition/mods/54909?tab=files
2. Extract the zip and place the files in your Skyrim install folder (Skyrim Special Edition\Data) 
   ### DO NOT USE A MOD MANAGER AS HOT CODE RELOADING WILL NOT WORK WITH THE VIRTUAL DRIVE.
3. Go to the `Skyrim Special Edition\Data\Platform\plugin-example` folder and run `yarn` on the command line to install the dependencies.
4. Create a file in the `Skyrim Special Edition\Data\Platform\plugin-example\tsc` folder named `config.js` with the following contents:
   ```
   module.exports = {
       // Change `seRoot` to the correct path to the Skyrim SE folder. The path should have slashes like this: `/` (not `\\`).
       seRoot: "C:/Program Files (x86)/Steam/steamapps/common/Skyrim Special Edition"
   };
   ```
5. Run `yarn dev`. If everything is ok, the message `Found 0 errors, installing plugin-example.js` will appear.
6. Log in to Steam and start the game with `skse64_loader.exe`.

The console should display the following output (console opens to `~` in game).
``,
Hello SE
[Script] Hello Platform
``,

7. Make sure `src/example.ts` is working. To do this, write `coc riverwood` on the command line. You will be teleported to the location. Talk to Fendal. You should take away 100 gold and the inscription `Thanks for support` will appear. 
8. Check that hot code reloading works by changing the debug text in `plugin-example.js` and adding more gold to your inventory (open console and run player.additem f 1000) and talking to Fendal again. Now you should see your changed text in game!
