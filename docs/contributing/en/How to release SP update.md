1. Run the SP Release action https://github.com/skyrim-multiplayer/skymp/actions/workflows/sp-release.yml. It will create a pull request, which usually takes a minute.
2. Open the generated pull request and check the grammar, style, and logic of the pull request. It should collect several text files into one and change the version number everywhere.
3. Accept the pull request. We are not afraid of anything! In the case of everything, everything rolls back very easily.
4. Run the SP Types Update action https://github.com/skyrim-multiplayer/skymp/actions/workflows/trigger-sp-types-update.yml. It usually takes less than a minute.
5. Make sure that the commit to the main branch https://github.com/skyrim-platform/skyrim-platform arrived here.
6. Wait for PR Windows, PR Windows AE actions to be built in the main branch.