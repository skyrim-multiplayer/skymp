# skyrimplatform-plugin-example

## Setting Up

1. Run `npm i` on the command line to install the dependencies.
2. Create a file `tsc/config.js` with the following contents:
   ```js
   module.exports = {
       // Change `seRoot` to the correct path to the Skyrim SE folder. The path should have slashes like this: `/` (not `\\`).
       seRoot: "C:/Program Files (x86)/Steam/steamapps/common/Skyrim Special Edition"
   };
   ```

3. Run `npm run dev`. If everything is ok, the message `Found 0 errors, installing plugin-example.js` will appear.
4. Log in to Steam and start the game with `skse64_loader.exe`.

The console should display the following output (console opens to `~` in game).
``,
Hello SE
[Script] Hello Platform
``,

Make sure `src/example.ts` is working. To do this, write `coc riverwood` on the command line. You will be teleported to the location. Talk to Fendal. You should take away 100 gold and the inscription `Thanks for support` will appear. This is a sample script.