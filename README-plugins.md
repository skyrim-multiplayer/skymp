# Plugin-System fuer den SkyMP-Server

Dieses Dokument beschreibt Einrichtung, Betrieb und Migration fuer das Plugin-System.

## Zweck

Das Plugin-System trennt den monolithischen Gamemode in kleinere Addons auf.

- Plugins werden beim Start aus `Plugins/` erkannt.
- Erkennung erfolgt ueber `plugin.json` (nicht ueber Ordnernamen).
- Neue Plugins werden nie automatisch aktiviert.
- Aktivierung erfolgt per Admin-Entscheidung (Prompt oder Panel-Workflow).

## Pfade (Windows/Linux)

Der Pfad ist immer relativ zum Server-Arbeitsverzeichnis (`process.cwd()`).

- Windows Beispiel: `Server\\Plugins\\ChatServer\\plugin.json`
- Linux Beispiel: `Server/Plugins/ChatServer/plugin.json`

Konvention:

- Ordnername ist exakt `Plugins` (Gross-/Kleinschreibung beibehalten).
- Es werden nur direkte Unterordner von `Plugins` gescannt.

## Ersteinrichtung und Discovery-State

Beim ersten Start erzeugt der Server eine State-Datei:

- `Plugins/.discovery-state.json`

Inhalt pro Plugin:

- `pluginPath`: Relativer Pfad des Plugin-Ordners
- `fingerprint`: Hash aus Manifest plus relevanten Entry-Daten
- `version`: Version aus dem Manifest
- `startupEnabled`: Automatisch beim Boot laden/starten
- `discoveredAt`, `updatedAt`: Zeitstempel

Die State-Datei trennt:

- bekannt
- neu
- aktualisiert
- entfernt (aus dem Plugin-Ordner verschwunden)

## plugin.json Mindestschema

Pflichtfelder:

- `name`: Eindeutiger Schluessel
- `version`: Versionsstring (empfohlen SemVer)
- `kind`: `gamemode` oder `process`
- `displayName`: Anzeigename fuer Operatoren
- `description`: Kurzbeschreibung fuer Entscheidung/Review
- `main`: Relativer Einstiegspunkt (Default im Loader: `index.cjs`)

Optionale Felder:

- `optional`: Bei Fehler nicht als kritischer Startfehler behandeln
- `startupDefault`: Dokumentationshinweis, wird bei neuen Plugins bewusst ignoriert
- `command`: Nur fuer `kind: process`, auszufuehrender Befehl
- `args`: Nur fuer `kind: process`, Argumentliste

### Komplettbeispiel: gamemode

```json
{
  "name": "emotewheel",
  "version": "1.2.0",
  "kind": "gamemode",
  "displayName": "Emote Wheel",
  "description": "Adds chat-driven emote wheel bindings",
  "main": "index.cjs",
  "optional": false
}
```

### Komplettbeispiel: process

```json
{
  "name": "chat-server",
  "version": "0.4.1",
  "kind": "process",
  "displayName": "Chat Server",
  "description": "Standalone chat bridge process",
  "main": "server.mjs",
  "command": "node",
  "args": ["server.mjs"],
  "optional": true
}
```

## Boot-Ablauf

1. Haupt-Gamemode aus `gamemodePath` laden.
2. `Plugins/` auf direkte Unterordner scannen.
3. Nur Ordner mit `plugin.json` einbeziehen.
4. Manifest validieren und Fingerprint berechnen.
5. Gegen `Plugins/.discovery-state.json` vergleichen.
6. Sortierung:
   - Erst `pluginsLoadOrder` aus Settings (falls vorhanden)
   - Danach alphabetisch nach `name`
7. Nur Plugins mit `startupEnabled=true` starten.
8. State-Datei aktualisieren.

`kind: gamemode`:

- Plugin-Entry via `require` laden.

`kind: process`:

- Prozess via `spawn` starten (`cwd` = Plugin-Ordner).
- Logs werden mit Prefix ausgegeben (`[plugin:<name>]`).
- Beim Serverende werden Kindprozesse beendet (Windows: `taskkill /t /f`, Linux: `SIGTERM`).

## Aktivierung neuer Plugins

Neue Plugins werden standardmaessig deaktiviert angelegt (`startupEnabled=false`).

Konfiguration in `server-settings.json`:

```json
{
  "pluginDiscovery": {
    "mode": "safe",
    "processCommandAllowlist": ["node", "node.exe"]
  },
  "pluginsLoadOrder": ["core", "chat-server", "emotewheel"],
  "abortOnPluginError": true
}
```

`pluginDiscovery.mode`:

- `safe`: Kein Prompt, neue Plugins bleiben deaktiviert
- `prompt`: Bei interaktivem Start Ja/Nein-Prompt fuer neu entdeckte Plugins

`pluginDiscovery.processCommandAllowlist`:

- Optionales String-Array fuer `kind: process` Plugins.
- Wenn gesetzt, duerfen nur diese Commands gestartet werden.
- Vergleich erfolgt gegen kompletten `command` oder dessen Dateinamen (z. B. `node`).
- Wenn nicht gesetzt, gibt es keine zusaetzliche Einschraenkung (Rueckwaertskompatibilitaet).

`abortOnPluginError` (default `false`):

- `true`: Server bricht den Start ab, wenn ein nicht-`optional` Plugin einen Fehler wirft. Empfohlen fuer Produktivumgebungen.
- `false`: Plugin-Fehler werden geloggt, Server startet dennoch.

Hinweis fuer Daemon-Betrieb:

- Mit `safe` starten und Freigabe per Panel/State-Workflow machen.

## Minimalbeispiele

### Minimal: gamemode-Plugin

Dateien:

- `Plugins/HelloPlugin/plugin.json`
- `Plugins/HelloPlugin/index.cjs`

`index.cjs`:

```js
mp.registerEvent("onInit", () => {
  console.log("[HelloPlugin] loaded");
});
```

### Minimal: process-Plugin

Dateien:

- `Plugins/ChatServer/plugin.json`
- `Plugins/ChatServer/server.mjs`

`server.mjs`:

```js
setInterval(() => {
  console.log("[ChatServer] heartbeat");
}, 10000);
```

## Sicherheit

Alles unter `Plugins/` kann Host-Code ausfuehren.

- Niemals unbekannte Plugins blind aktivieren.
- Neue Plugins nur nach Review auf `startupEnabled=true` setzen.
- `plugin.json` plus Entry-Dateien als trusted code behandeln.
- Fuer `kind: process` Plugins in Produktion `pluginDiscovery.processCommandAllowlist` setzen (z. B. nur `node`/`node.exe`).

## Troubleshooting

- Plugin wird nicht erkannt:
  - Liegt `plugin.json` direkt in `Plugins/<Name>/`?
  - Sind Pflichtfelder gesetzt?
- Plugin startet nicht:
  - `main` pruefen (`kind: gamemode`)
  - `command` und `args` pruefen (`kind: process`)
  - Logs mit Prefix `[plugins]` und `[plugin:<name>]` kontrollieren
- Prompt erscheint nicht:
  - `pluginDiscovery.mode` ist `prompt`?
  - Start ist interaktiv (TTY)?
- Prozess bleibt haengen:
  - Auf Windows wird `taskkill /t /f` verwendet
  - Auf Linux `SIGTERM` (ggf. eigenen Handler im Plugin implementieren)

## Runtime-Status im Admin-API

`GET /api/admin/plugins` enthaelt zusaetzlich `runtimeStatus` pro Plugin:

- `starting`: Plugin-Start wurde initiiert
- `running`: Plugin laeuft
- `failed`: Start oder Laufzeitfehler erkannt
- `stopped`: Nicht gestartet (`startupEnabled=false`) oder sauber beendet

Beispielauszug:

```json
{
  "name": "chat-server",
  "startupEnabled": true,
  "runtimeStatus": {
    "name": "chat-server",
    "status": "running",
    "pid": 12345,
    "updatedAt": 1760000000000
  }
}
```

## Migration vom monolithischen gamemode

1. Bisherige Module in einzelne Plugin-Ordner verschieben.
2. Pro Plugin ein `plugin.json` erstellen.
3. `gamemodePath` als Bootstrap/Haupt-Gamemode belassen.
4. Erst mit `pluginDiscovery.mode = safe` starten.
5. Neue Plugins einzeln freigeben (`startupEnabled=true`).
6. Optional Reihenfolge ueber `pluginsLoadOrder` festlegen.

## Voice Activity Adapters (für Lipsync & Proximity Chat)

Das Voice Activity Detection (VAD) System nutzt ein **Adapter-Pattern** für verschiedene Voice-Anbieter.

### Was ist ein Voice Adapter?

Ein Voice Adapter ist ein einfaches Plugin, das:
1. Voice-Events von einem externen System empfängt (TeamSpeak, Discord, custom VoIP)
2. Events ins standardisierte SkyMP-Format konvertiert
3. An den Server Voice Activity Manager sendet
4. Der Manager broadcastet dann die Lipsync-Animation zu Clients

### Vorteile des Adapter-Patterns

- **Provider-neutral**: YACA-TS, Discord, Mumble, custom 3D-VoIP — alle über die gleiche API
- **Keine Server-Code-Changes**: Neue Voice-Systeme brauchen nur einen neuen Adapter
- **Modulare Health-Tracking**: Admin-Dashboard zeigt Status pro Voice-Provider
- **Asynchron erweiterbar**: Dritte können neue Adapter schreiben, ohne Kernel zu verändern

### YACA-TS Reference Implementation

Die YACA-TS Adapter ist in `skymp5-server/ts/adapters/YacaTeamSpeakAdapter.ts` definiert.

**Funktionsweise:**
1. YacaBridgeQuest.psc (Papyrus) empfängt TeamSpeak YACA-Plugin-Events
2. Sendet via `mp.callGamemodeApi()` custom packet an Server
3. Adapter empfängt Packet und konvertiert zu `VoiceActivityState`
4. Voice Activity Manager broadcastet zu Proximity-Clients
5. Clients spielen IdleDialogueLock Animation während Player spricht

**Konfiguration (server-settings.json):**
```json
{
  "voiceActivity": {
    "enabled": true,
    "providers": ["yaca-ts"],
    "voiceRangeTiers": [1.0, 3.0, 8.0, 15.0, 20.0, 25.0, 30.0, 40.0],
    "defaultVoiceRange": 8.0,
    "proximityDistance": 100.0,
    "inactivityTimeoutMs": 1000
  }
}
```

### Custom Voice Adapter Erstellen

Siehe `docs/docs_voice_api.md` für vollständige API-Dokumentation und Beispiele.

**Minimales Beispiel (Discord Bot Adapter):**

```typescript
import { IVoiceProviderAdapter, VoiceActivityState } from "../voiceActivityApi";

export class DiscordVoiceAdapter implements IVoiceProviderAdapter {
  readonly providerId = "discord-bot";
  private callbacks: Set<(state: VoiceActivityState) => void> = new Set();
  
  async initialize(): Promise<void> {
    // Verbinde zu Discord Bot API
    // Subscribiere zu Voice State Update Events
    console.log("[Discord Adapter] Initialized");
  }
  
  async shutdown(): Promise<void> {
    // Cleanup
  }
  
  onVoiceActivityUpdate(callback: (state: VoiceActivityState) => void): void {
    this.callbacks.add(callback);
  }
  
  // Wird aufgerufen, wenn Discord Bot User Voice State ändert
  handleDiscordVoiceEvent(userId: string, inVoiceChannel: boolean): void {
    const state: VoiceActivityState = {
      actorId: this.mapDiscordIdToActorId(userId),
      isSpeaking: inVoiceChannel,
      voiceRange: 8.0,
      providerId: this.providerId,
      timestamp: Date.now(),
    };
    
    for (const cb of this.callbacks) {
      cb(state);
    }
  }
  
  getHealthStatus() {
    return {
      isHealthy: true,
      uptime: process.uptime() * 1000,
      activeSpeakers: 0,
      lastEventTimestamp: 0,
      errors: [],
    };
  }
  
  // Hilfsmethode: Discord User ID → Skyrim Actor ID Mapping
  private mapDiscordIdToActorId(discordId: string): string {
    // TODO: Implementiere Mapping-Logic
    // Optionen: JSON-Datei, Datenbank, oder Redis-Cache
    return "0x00000000"; 
  }
}
```

### Admin Panel für Voice Status

Das Admin-Dashboard zeigt pro Adapter:
- **Health Status**: 🟢 Running, 🔴 Failed, 🟡 Starting
- **Active Speakers**: Aktuelle Sprecherzahl
- **Uptime**: Wie lange Adapter aktiv ist
- **Errors**: Letzte Fehler

API-Endpoint: `GET /api/admin/voice/status`

```json
{
  "isEnabled": true,
  "activeSpeakers": 3,
  "registeredAdapters": [
    {
      "providerId": "yaca-ts",
      "isHealthy": true,
      "uptime": 3600000
    },
    {
      "providerId": "discord-bot",
      "isHealthy": false,
      "uptime": 1800000
    }
  ],
  "lastBroadcastTime": 1715691234567,
  "errors": []
}
```

### Troubleshooting Voice Adapters

**Problem: Adapter lädt nicht**
- Check: `GET /api/admin/voice/status` → `errors[]`
- Verifiziere Provider ist in `allowedProviders` oder `allowedProviders=null`
- Logs checken: `[Voice Activity Manager] Registered provider: ...`

**Problem: Lipsync funktioniert nicht**
- Verifiziere Adapter sendet Events: `getHealthStatus()` → `lastEventTimestamp`
- Check Client-seitige Animation Handler sind aktiv
- Teste mit Admin API: `GET /api/admin/voice/speakers` → sollte aktive Speaker zeigen

**Problem: Hohe Latenz zwischen Voice und Lipsync**
- Reduce `inactivityTimeoutMs` in Config (aktuell 1000ms)
- Check Proximity-Filter: `proximityDistance` könnte zu klein sein
- Monitor `lastBroadcastTime` in Admin Status

### Weitere Ressourcen

- Komplette Voice API Spec: `docs/docs_voice_api.md`
- YACA-TS Repo: `https://github.com/skyrim-multiplayer/SkyMP-YACA-TS`
- Reference Adapter: `skymp5-server/ts/adapters/YacaTeamSpeakAdapter.ts`
- Roadmap: `ROADMAP.md` → Voice & Proximity Chat Backlog
