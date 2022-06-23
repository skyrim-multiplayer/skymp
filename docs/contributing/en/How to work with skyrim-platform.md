1. While in the `build` folder, generate the project with `cmake .. -DSWEETPIE=OFF`. Turning off SweetPie is necessary to make it easier to enter the server.
2. `cmake --build . --config Release` to build the entire project.
3. Go to `build` and double-click on `skymp.sln`. Solution will open in the studio.
4. In `cmd` go to `skyrim-platform/tools/dev_service` and write `npm run dev`
