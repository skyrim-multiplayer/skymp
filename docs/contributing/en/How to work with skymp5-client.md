1. The path to the repository must be `C:/projects/skymp`, otherwise the instruction will not work. It will be fixed, but for now.
2. While in the `build` folder, generate the project with `cmake .. -DSWEETPIE=OFF -DSKYRIM_DIR="<folder path>"`. Turning off SweetPie is necessary to make it easier to enter the server. Specifying the path to Skyrim is necessary for the server to start in principle - it needs master files.
3. `cmake --build . --config Release` to build the entire project.
4. Call `code .` at the root of the project. We will be working mainly with `skymp5-client`, but we may need to look in other folders as well, so open the whole project.
5. `cd build/dist/server`, `launch_server.bat`
6. Copy `build/dist/client` to the root of the game. Delete the contents of `Data/Platform/Plugins`. In our case, SP will not use this folder, but `build/dist/client/Data/Platform/Plugins`. This is required for hotreload to work.
7. In VS Code go to `skymp5-client/package.json`. In the scripts section, RMB on watch -> RunScript. Now, for any of our changes, skymp5-client will be rebuilt and pulled into the game.
8. Run the game using `skse64_loader.exe`.
9. You may need to see your clone to test synchro. To do this, write "show-me": true in `build/dist/client/skymp5-client-settings.txt`. Reboot is not required.
10. You can also set "show-net-info": true to diagnose packages.

You don't need to build with cmake and keep watch running at the same time, otherwise skymp5-client will be damaged, it will throw scary and meaningless exceptions at the game console that you cannot fix. Solved by turning off and on watch.

(TODO: Now this feature is actually available only to me, because the path is hardcoded. We need to support the configuration of this path, perhaps through the platform config, when we merge the AE branch. By the way, when building, we know the path to the dist, obviously. You can do it automatically.)
