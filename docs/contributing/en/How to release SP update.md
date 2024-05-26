1. Run the SP Release action https://github.com/skyrim-multiplayer/skymp/actions/workflows/sp-release.yml. It will create a pull request, which usually takes a minute.
2. Open the generated pull request and check the grammar, style, and logic of the pull request. It should collect several text files into one and change the version number everywhere.
3. Accept the pull request. We are not afraid of anything! In the case of everything, everything rolls back very easily.
4. Run the SP Types Update action https://github.com/skyrim-multiplayer/skymp/actions/workflows/trigger-sp-types-update.yml. It usually takes less than a minute.
5. Make sure that the commit to the main branch https://github.com/skyrim-platform/skyrim-platform arrived here.
6. Wait for PR Windows, PR Windows AE actions to be built in the main branch.
7. Download `Skyrim Platform 2.X.0 (Anniversary Edition).zip` and `Skyrim Platform 2.X.0 (Special Edition).zip` from artifacts.
8. Go to file management on Nexus https://www.nexusmods.com/skyrimspecialedition/mods/edit/?id=54909&game_id=1704&step=files, scroll down to "Add a new file" and fill in all the items:

File name: `Skyrim Platform 2.X.0 (Anniversary Edition)`

File version: `2.X.0`

This is the latest version of the mod: `✔️`

File category: `Main files`

This is a new version of an existing file (optional): `Yes it is!`

Description:
```
THIS IS FOR ANNIVERSARY EDITION (1.6.x)!

Please backup your saves before updating. There are issues with some modlists.

Changelog: https://github.com/skyrim-multiplayer/skymp/blob/main/docs/release/sp-2.XXXXXXXXX.md
For questions: https://discord.gg/k39uQ9Yudt
```

9. Attach the file. "Save your file".
10. Go to file management on Nexus https://www.nexusmods.com/skyrimspecialedition/mods/edit/?id=54909&game_id=1704&step=files, scroll down to "Add a new file" and fill in all the items:

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

11. Attach the file. "Save your file".
