# Upstream-Abgleich vom 18. Oktober 2026

## Verglichene Stände

Die Remote-Referenzen von `origin` und `upstream` wurden vor dem Vergleich frisch abgerufen.
Ein erneuter Abruf am 19. Oktober 2026 bestätigte dieselben Remote-Stände.

| Stand | Commit |
| --- | --- |
| Fork `origin/main` und ursprünglicher lokaler `main` | `25b82485e8dd901ef032e46ad8f2b873114cadc1` |
| Haupt-Repository `upstream/main` | `2849e676fe008925384b26f34476e382a30bd877` |
| Arbeitsbranch für die Integration | `sync/upstream-20261018` |

Dem Fork fehlten 29 Upstream-Commits. Gleichzeitig enthielt er 59 zusätzliche
Commits einschließlich früherer Merges. Die letzte Upstream-Änderung stammt vom
5. Oktober 2026. Der Vergleich bezieht sich auf die jeweiligen `main`-Branches.

Beim Beginn der Prüfung war ein Rebase nach dem zweiten von 51 Schritten mit
Konflikten unterbrochen. Der vollständige angefangene Stand einschließlich
Index, Rebase-Metadaten, Patches und betroffenen Dateien wurde lokal unter
`build/fork-sync-backup-20261018/` gesichert. Anschließend wurde der ursprüngliche
Fork wiederhergestellt und Upstream auf einem eigenen Branch zusammengeführt.
Der ursprüngliche `main` und `backup-sync-20261018123519` bleiben erhalten.

## Auswirkungen und Anpassungen

| Bereich | Auswirkung und Lösung |
| --- | --- |
| Eigene Erweiterungen | Admin-Dashboard, Mehrsprachigkeit, Serverliste, Respawn, Favoriten, Effekte, Verzauberungen, Marker und YACA/Voice bleiben im Fork enthalten. |
| Gamemode-API | `getMyFormIdInServerFormat()` wurde in Interface, Event-Sources, Owner-/Nachbar-Updates und Eval-Kontext ergänzt. |
| SkyrimPlatform | Upstream entfernt die alte System-Polyfill-Laufzeit und SkyrimPlatform-Plugins aus mehreren Dateien auf dem Client. Der eigene Client wird bereits als einzelne Webpack-Datei gebaut. Externe ältere Client-Plugins müssen ebenfalls gebündelt werden. Der serverseitige Plugin-Manager bleibt erhalten. Die alte Polyfill-Datei gehört nicht mehr in neue Distributionen. |
| Engine-Korrekturen | Upstream behebt das Plugin-Laden bei fehlenden Plugin-Ordnern und einen möglichen Hänger beim Beenden des Clients. Dazu kommen LCTN-/CELL-Datenzugriff und das Laden von ACTI-/FURN-Objekten. Die Auswirkungen auf eine konkrete Spielwelt müssen im Spiel geprüft werden. |
| Generierte API | Der neue JavaScript-Generator behält die zusätzlichen Fork-Deklarationen für `Browser.getToken`, `ObjectReference.addItem` und `Actor.addToFaction` bei. `markerFormId` und `showMapMarker` bleiben ebenfalls erhalten. Der von Upstream entfernte Enum-Alias `EquippedItemType.Spell` wird durch `SpellOrScroll` ersetzt. |
| Bestehende Build-Ordner | Der Client-Paketierer verwendet die aktuelle generierte API im Quellbaum. Zurückgebliebene `_codegen/skyrimPlatform.ts`-Dateien des entfernten nativen Generators überschreiben weder die neue API noch deren Kopie im Paket. |
| Kartenmarker | Der native Callback für `TES::ForEachReference` verwendet den von CommonLibSSE erwarteten `TESObjectREFR*` und überspringt leere Zeiger. Die zusätzliche `markerFormId` im `locationDiscovery`-Ereignis bleibt erhalten. |
| Server-Abhängigkeiten | Die Upstream-Anforderungen für axios, ws und esbuild wurden übernommen und das npm-Lockfile aktualisiert. Das zusätzliche OpenTelemetry-Shim des Forks bleibt aktiv. |
| Client-Einstellungen | Die neue CMake-Option `GENERATE_CLIENT_SETTINGS` wurde mit der vorhandenen Fork-Erzeugung zusammengeführt. `OFF` verhindert das Erzeugen und Überschreiben von Einstellungen, die bereits ein Installer eingerichtet hat. |
| Deployments | Die neuen Upstream-Workflows verwenden die Repository-Variable `INSTALLER_REPOSITORY` statt eines fest eingebauten Upstream-Ziels. Settings-Workflows ermitteln den Standardbranch des Ziel-Repositories. Der bisherige `repository_dispatch`-Eingang `deploy` bleibt für `deploy` und `restart` erhalten. |
| Fork-Workflows | Installer-Dispatch und SSH-Logabruf bleiben erhalten. Das für den Logabruf benötigte Shell-Skript wurde ebenfalls beibehalten. Docker-Tag-Updates verweisen auf die weiterhin vorhandene Umgebungsdatei. |
| Oberfläche | Ein bereits vorhandener Startfehler im Admin-Dashboard wurde behoben: Die Validierung von Moderationsgründen erfolgt nach der Initialisierung der zugehörigen Zustandsvariablen. Ein Regressionstest deckt den Start des unsichtbaren Dashboards ab. |
| Admin-Daten | Der Berechtigungs-Snapshot berücksichtigt `canWarn`, der Status-Typ enthält die Master-Adresse und der Ereignisvergleich verwendet die tatsächlich vorhandenen Felder statt eines nicht existierenden `title`. |
| Admin-WebSocket | Veränderte React-Callbacks lösen keine neuen Verbindungen mehr aus. Eine Ref übergibt Nachrichten an den aktuellen Handler. Auch noch nicht geöffnete Verbindungen werden beim Schließen aufgeräumt; späte Ereignisse alter Verbindungen werden ignoriert. |
| Serverpaket | Neben `build.js` werden alle erzeugten Frontend-Assets in `dist/server/ui` übernommen, damit Bilder, Schriften und Sounds auch in frischen Paketen vorhanden sind. |
| Windows-Build | CMake berücksichtigt zusätzlich übergebene `VCPKG_INSTALL_OPTIONS`. Damit lassen sich temporäre Abhängigkeits-Builds und Werkzeugdownloads auf NTFS auslagern; die bisherigen Cleanup-Vorgaben bleiben wirksam. |
| Browser-Test | Der Smoke-Test wartet auf das Mounten von React und scheitert bei fehlender Serverliste, statt alle Launcher-/Admin-Prüfungen mit erfolgreichem Exit zu überspringen. Er prüft außerdem stabile WebSocket-Verbindungen bei UI-Updates und die Verarbeitung des aktualisierten Serverstatus. |
| C++-Testdaten | Bewegungsnachrichten enthalten das im Fork erforderliche `isMounted`. NPC-Tests aktivieren ihre expliziten Spawn-Regeln mit `overriden`, entsprechend dem vorhandenen Konfigurationsmodell des Forks. |

## Konfiguration für GitHub-Deployments

Im Fork unter **Settings → Secrets and variables → Actions**:

- Variable `INSTALLER_REPOSITORY`: das eigene Installer-Repository als `owner/repo`.
- Secret `SKYMP5_INSTALLER_REPOSITORY_DISPATCH_PAT`: Token mit Zugriff auf dieses Repository und Berechtigung zum Auslösen seiner Workflows.

Die übernommenen Deployments erwarten dort `server-build-deploy-1-5.yml` bzw.
`server-settings-1-0.yml`. Der bestehende Installer-Dispatch verwendet weiterhin
das Ereignis `on-skymp-push`. Ohne passendes Installer-Repository sind diese
optionalen Deployment-Aktionen nicht ausführbar; lokale Builds benötigen es nicht.

Upstream ersetzt die bisherigen SSH-Deployments durch Installer-Workflows und
entfernt die Deploy-Auswahl `stop`. Alte Aufrufer mit `action: stop` oder dem
Schutzparameter `canaryFile` müssen angepasst werden; der neue Workflow weist
solche Aufträge ausdrücklich zurück, statt einen bisherigen Schutzparameter
stillschweigend zu ignorieren. Der SSH-Logabruf des Forks bleibt verfügbar.

## Windows-Arbeitsverzeichnisse

Das lokale Laufwerk `D:` verwendet exFAT, `C:` verwendet NTFS. Beim nativen
Build scheiterte CMake 4.4.2 mehrfach am Wiederherstellen von Zeitstempeln beim
Entpacken von Werkzeug-ZIPs. Meson brach anschließend wegen eines frisch
geschriebenen, in die Zukunft gerundeten Zeitstempels ab. Ein separater
Dateischreibtest bestätigte die Rundung auf dem Repository-Laufwerk.

Die temporären vcpkg-Verzeichnisse werden deshalb auf NTFS gelegt. Der
Quellcode, bereits installierte Abhängigkeiten und die Distribution bleiben
im bisherigen Repository. In PowerShell aus `build/` konfigurieren:

```powershell
$nativeWorkRoot=(Join-Path $env:TEMP 'skymp-native-vcpkg-20261018').Replace('\','/')
cmake .. "-DVCPKG_INSTALL_OPTIONS:STRING=--x-buildtrees-root=$nativeWorkRoot/buildtrees;--x-packages-root=$nativeWorkRoot/packages;--downloads-root=$nativeWorkRoot/downloads" -DGENERATE_CLIENT_SETTINGS:BOOL=OFF
```

Die letzte Option erhält die vorhandenen Client-Einstellungen. Für Tests
mit den echten Skyrim-Masterdateien zusätzlich `-DUNIT_DATA_DIR:PATH=...`
mit dem vorhandenen Data-Verzeichnis übergeben. Diese Tests lesen die
Masterdateien; sie verwenden nicht die gespeicherte Serverwelt.

## Lokale Prüfungen

Erfolgreich geprüft:

- Client-Webpack-Build und Voice-Sync-Pakettest.
- TypeScript-Server einschließlich Typprüfung und esbuild-Bundle.
- Frontend-Build und vollständige TypeScript-Prüfung.
- Sprachkonsistenz, Frontend-Unit-Tests einschließlich des neuen Dashboard-Starttests und API-Integrationstests mit simulierten Antworten.
- Playwright-Ablauf am frisch gebauten Frontend mit simulierten HTTP-/WebSocket-Antworten: Launcher, Favoriten/Verbindung, Admin-Moderation, Warnungen, Respawn, Ereignisse, Spielerhistorie und stabile WebSocket-Updates. Keine unbehandelten Browserfehler.
- Deterministische Codegenerierung mit Erhalt der fünf geprüften Fork-API-Deklarationen.
- Workflow-YAML und Bash-Syntax von 43 Linux-Schritten.
- Vollständiger nativer Windows-Release-Build einschließlich SkyrimPlatform, Server-Modul und C++-Testprogramm mit Visual Studio 2026.
- Alle 71 Frontend-Dateien im tatsächlich erzeugten Serverpaket stimmen per SHA-256 mit dem frischen Frontend-Build überein.
- Native Server-Integrationstests `test_removeallitems`, `test_onactivate` und `test_factions` in einer separaten temporären Distribution mit Kopien der fünf Skyrim-Masterdateien.
- HTTP-Test mit echtem Server und nativem Modul: authentifizierter Admin-Status, HTTP 401 ohne Anmeldung, bytegenaue Auslieferung von `/build.js` und sauberes Beenden des Test-Gamemodes. Ein Test-Preload unterbindet ausschließlich den Probezugriff auf den bereits vorhandenen lokalen UI-Entwicklungsserver an Port 1234.

- Vollständiger C++-Unit-Lauf über `ctest -C Release -R '^test_unit$' --output-on-failure`: alle 265 Testfälle mit 476.763 Assertions erfolgreich, 46,38 Sekunden. Vorher wurden der Bewegungs-Test sowie alle zehn NPC-/Template-/Aktivierungstests einzeln bzw. als Gruppe erfolgreich wiederholt. Die Speicher-Testgruppe bestand auch mit dem Zufallswert des ersten fehlgeschlagenen Laufs.

Die ESPM-Tests verwendeten die fünf vorhandenen Skyrim-Masterdateien. Prüfungen,
die ausdrücklich die CI-Umgebung bzw. feste Installations-Hashes voraussetzen,
werden vom Testprogramm lokal übersprungen. Ein echter Spieltest mit verbundenem
Skyrim-Client ist weiterhin offen.

Vorhandene Server- und Client-Einstellungen wurden gesichert und erhalten.
Der lokale CMake-Cache verwendet die vorhandene Master-Adresse und den
vorhandenen Offline-Modus; `GENERATE_CLIENT_SETTINGS=OFF` schützt die
Client-Einstellungen. Die bestehende Serverwelt wurde für die Integrationstests
nicht verwendet. Testprotokolle und das isolierte PowerShell-Testskript liegen
unter `build/fork-sync-*` und sind lokale, nicht versionierte Prüfartefakte.

Die Integration ist auf `sync/upstream-20261018` lokal vorbereitet. Es wurde
nichts zu GitHub gepusht; `main` und `origin/main` stehen weiterhin auf dem
ursprünglichen Fork-Commit. Der Integrationsbranch enthält diesen Commit und
den aktuellen Upstream-Commit vollständig als Vorfahren.

Die nativen Integrationstests setzen `server-settings.json` und `gamemode.js`
zurück und löschen `world/changeForms` in ihrer Server-Distribution. Sie dürfen
deshalb nicht ungesichert gegen die vorhandene Entwicklungswelt laufen. Für
einen vollständigen Gameplay-Test sind eine separate Testwelt und ein laufendes
Skyrim mit dem neu gebauten Client erforderlich.

## Wiederholen der Prüfungen

```powershell
npm --prefix skymp5-front test
npm --prefix skymp5-front run test:api
npm --prefix skymp5-front run build
npm --prefix skymp5-client run build
npm --prefix skymp5-client run test:voice-sync
npm --prefix skymp5-server run build-ts

node skyrim-platform/tools/ts_converter/index.js
yarn --cwd skyrim-platform/tools/const_enum_extractor install --frozen-lockfile
node skyrim-platform/tools/const_enum_extractor/index.js

cd build
cmake --build . --config Release --parallel 4
ctest -C Release -R '^test_unit$' --output-on-failure
```

Für den Browser-Test das gebaute Frontend bzw. den Frontend-Entwicklungsserver
starten und `PLAYWRIGHT_E2E_URL` auf dessen URL mit `?devUi=1` setzen. Danach
`npm --prefix skymp5-front run test:e2e` ausführen. Die API-Antworten dieses Tests
sind simuliert; er ersetzt keinen Test mit verbundenem Spielclient.
