1. Находясь в папке `build`, сгенерируйте проект с помощью `cmake .. -DSWEETPIE=OFF`. Выключение SweetPie нужно, чтобы облегчить вход на сервер.
2. `cmake --build . --config Release`, чтобы собрать проект целиком.
3. Заходим в `build` и запускаем двойным кликом `skymp.sln`. Солюшон откроется в студии.
4. В `cmd` заходим на `skyrim-platform/tools/dev_service` и пишем `npm run dev`
