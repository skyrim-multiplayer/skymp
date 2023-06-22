1. Запустить экшон SP Release https://github.com/skyrim-multiplayer/skymp/actions/workflows/sp-release.yml. Он создаст пулл реквест, обычно это занимает минуту.
2. Открыть созданный пулл реквест и проверить грамматику, стиль, а также логику пулл реквеста. Он должен собирать несколько текстовых файлов в один и менять везде цифру версии.
3. Принять пулл реквест. Ничего не боимся! В случае всего всё очень легко откатывается.
4. Запустить экшон SP Types Update https://github.com/skyrim-multiplayer/skymp/actions/workflows/trigger-sp-types-update.yml. Он обычно выполняется не более минуты.
5. Убедиться, что сюда прилетел коммит в main ветку https://github.com/skyrim-platform/skyrim-platform.
6. Ждать, пока соберётся PR Windows, PR Windows AE экшоны в мейн ветке.
7. Скачать SP-AE.zip и SP-SE.zip из артефактов. Переименовать их в `Skyrim Platform 2.X.0 (Anniversary Edition)` и `Skyrim Platform 2.X.0 (Special Edition)`.
8. Зайти в управление файлами на Nexus https://www.nexusmods.com/skyrimspecialedition/mods/edit/?id=54909&game_id=1704&step=files, пролистать до "Add a new file" и заполнить все пункты:

File name: `Skyrim Platform 2.X.0 (Anniversary Edition)`

File version: `2.X.0`

This is the latest version of the mod: `✔️`

File category: `Main files`

This is a new version of an existing file (optional): `❌`

Description:
```
THIS IS FOR ANNIVERSARY EDITION (1.6.x)!

Please backup your saves before updating. There are issues with some modlists.

Changelog: https://github.com/skyrim-multiplayer/skymp/blob/main/docs/release/sp-2.XXXXXXXXX.md
For questions: https://discord.gg/k39uQ9Yudt
```

9. "Attach the file". "Save your file".
10. Зайти в управление файлами на Nexus https://www.nexusmods.com/skyrimspecialedition/mods/edit/?id=54909&game_id=1704&step=files, пролистать до "Add a new file" и заполнить все пункты:

File name: `Skyrim Platform 2.X.0 (Special Edition)`

File version: `2.X.0`

This is the latest version of the mod: `✔️`

File category: `Main files`

This is a new version of an existing file (optional): `❌`

Description:
```
THIS IS FOR SPECIAL EDITION (1.5.x)!

Please backup your saves before updating. There are issues with some modlists.

Changelog: https://github.com/skyrim-multiplayer/skymp/blob/main/docs/release/sp-2.XXXXXXXXX.md
For questions: https://discord.gg/k39uQ9Yudt
```

11. "Attach the file". "Save your file".
