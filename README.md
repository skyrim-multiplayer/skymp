# SkyMP

[![Discord Chat](https://img.shields.io/discord/699653182946803722?label=Discord&logo=Discord)](https://discord.gg/k39uQ9Yudt)
[![PR's Welcome](https://img.shields.io/badge/PRs%20-welcome-brightgreen.svg)](CONTRIBUTING.md)
[![Players](https://skymp-badges.vercel.app/badges/players_online.svg)](https://discord.gg/k39uQ9Yudt)
[![Servers](https://skymp-badges.vercel.app/badges/servers_online.svg)](https://discord.gg/k39uQ9Yudt)

SkyMP is an open-source multiplayer project for Skyrim.

This repository is the main home of the project: server, client, admin web UI, build system, docs, and CI pipelines are all managed here.

## Project Status (WIP Fork)

This fork is currently a **work in progress**.

Some features are already usable, but not every part has been tested on every host setup yet. This is one reason why this fork is not integrated into the main repository at the moment.

We are actively looking for testers who can:

1. Set up their own servers on different systems.
2. Validate gameplay and admin workflows in real environments.
3. Report bugs, crashes, and configuration issues.

Because every system is different, real-world feedback is extremely valuable.

## Language Quick Links

| Language | Start Here |
| --- | --- |
| Deutsch | [Zum deutschen Abschnitt](#deutsch) |
| English | [Go to English section](#english) |
| Русский | [Перейти к русскому разделу](#русский) |
| Español | [Ir a la sección en español](#español) |
| Français | [Aller a la section francaise](#francais) |
| Italiano | [Vai alla sezione italiana](#italiano) |

## Deutsch

### Überblick

SkyMP bringt Multiplayer nach Skyrim. Dieses Repository ist die zentrale Anlaufstelle für Betrieb, Entwicklung und Beiträge.

### Wofür du dieses Repository nutzen kannst

1. Deinen eigenen Server betreiben.
2. Client und Server selbst bauen.
3. Mit Code, Dokumentation und Tests beitragen.

### Schnellstart

#### Server starten

Die einfachste Möglichkeit ist das direkte Herunterladen eines fertigen Builds – **kein Klonen des Repos, kein Kompilieren erforderlich**:

1. Lade das neueste Release von [github.com/skyrim-multiplayer/skymp/releases](https://github.com/skyrim-multiplayer/skymp/releases) herunter.
	- Windows: `running_server_files_windows_server_dist.zip`
	- Linux: `running_server_files_linux_server_dist.tar.gz`
2. Entpacke das Archiv in einen eigenen Ordner.
3. Starte `launch_server.bat` (Windows) oder `launch_server.sh` (Linux). Node.js und npm-Abhängigkeiten werden beim ersten Start automatisch installiert.
4. Passe `server-settings.json` an (Name, Ports, Load-Order).
5. Öffne das Admin-Dashboard unter `http://<host>:<uiPort>/admin`.

Vollständige Anleitung: [docs/docs_running_a_server.md](docs/docs_running_a_server.md).

#### Aus dem Quellcode bauen (für Entwickler)

1. Folge [CONTRIBUTING.md](CONTRIBUTING.md).
2. Baue das Projekt.
3. Artefakte findest du unter `build/dist`.
4. Server unter `build/dist/server`.
5. Client unter `build/dist/client`.

#### Beitreten als Spieler

1. Installiere Skyrim SE oder AE.
2. Installiere SKSE gemäß [docs/docs_client_installation.md](docs/docs_client_installation.md).
3. Falls du selbst baust: kopiere `build/dist/client` in deinen Skyrim-Ordner.
4. Starte das Spiel und verbinde dich mit deinem Server.

Wenn du keinen eigenen Launcher bauen möchtest, ist dieses Nexus-Tool eine praktische Option:

https://www.nexusmods.com/skyrimspecialedition/mods/30379?tab=files

Zusätzlich arbeiten wir an einem globalen Launcher, damit Server-Betreiber langfristig keinen eigenen Launcher mehr erstellen müssen.

### Plattformstatus

- Windows-Server: stabil und gut unterstützt.
- Linux-Server: empfohlen auf Ubuntu 24.04 und ähnlichen glibc-basierten Distributionen.
- Client: Schwerpunkt weiterhin Windows.

### Wichtige Bereiche im Repository

- `skymp5-server`: Server-Logik und Admin-API.
- `skymp5-front`: Admin-Dashboard.
- `skymp5-client`: Client-Integration.
- `docs`: Dokumentation.
- `cmake`, `overlay_ports`, `overlay_triplets`, `vcpkg`: Build- und Dependency-Infrastruktur.

### Wichtige Dokumente

- Einstieg in Build und Entwicklung: [CONTRIBUTING.md](CONTRIBUTING.md)
- Serverbetrieb: [docs/docs_running_a_server.md](docs/docs_running_a_server.md)
- Client-Installation: [docs/docs_client_installation.md](docs/docs_client_installation.md)
- Server-Konfiguration: [docs/docs_server_configuration_reference.md](docs/docs_server_configuration_reference.md)
- Ports und Netzwerk: [docs/docs_server_ports_usage.md](docs/docs_server_ports_usage.md)
- Projektstruktur: [docs/docs_repository_structure.md](docs/docs_repository_structure.md)

### Rechtliches

- Nutzungsbedingungen: [TERMS.md](TERMS.md)
- Drittanbieter-Lizenzen: [THIRD_PARTY_LICENSES](THIRD_PARTY_LICENSES)

## English

### Overview

SkyMP brings multiplayer to Skyrim. This repository is the central place for running servers, building binaries, and contributing to the project.

### What you can do here

1. Host your own server.
2. Build client and server from source.
3. Contribute code, docs, and tests.

### Quick Start

#### Run a server

The fastest way is to download a pre-built package — **no need to clone the repo or compile anything**:

1. Download the latest release from [github.com/skyrim-multiplayer/skymp/releases](https://github.com/skyrim-multiplayer/skymp/releases).
	- Windows: `running_server_files_windows_server_dist.zip`
	- Linux: `running_server_files_linux_server_dist.tar.gz`
2. Extract the archive to a dedicated folder.
3. Run `launch_server.bat` (Windows) or `launch_server.sh` (Linux). Node.js and npm dependencies are installed automatically on first start.
4. Edit `server-settings.json` to set your server name, ports, and load order.
5. Open the admin dashboard at `http://<host>:<uiPort>/admin`.

Full guide: [docs/docs_running_a_server.md](docs/docs_running_a_server.md).

#### Build from source (for contributors)

1. Follow [CONTRIBUTING.md](CONTRIBUTING.md).
2. Build the project.
3. Output is in `build/dist`.
4. Server package: `build/dist/server`.
5. Client package: `build/dist/client`.

#### Join as a player

1. Install Skyrim SE or AE.
2. Install SKSE as described in [docs/docs_client_installation.md](docs/docs_client_installation.md).
3. If you build locally, copy `build/dist/client` into your Skyrim folder.
4. Launch the game and connect to your server.

If you do not want to build your own launcher, this Nexus tool is a practical option:

https://www.nexusmods.com/skyrimspecialedition/mods/30379?tab=files

In parallel, we are working on a global launcher so server owners will not need to build and maintain separate custom launchers.

### Platform status

- Windows server: stable and well supported.
- Linux server: best on Ubuntu 24.04 and similar glibc-based distributions.
- Client runtime: still primarily Windows-focused.

### Key areas in this repo

- `skymp5-server`: server runtime and admin APIs.
- `skymp5-front`: admin dashboard frontend.
- `skymp5-client`: client-side integration.
- `docs`: full documentation.
- `cmake`, `overlay_ports`, `overlay_triplets`, `vcpkg`: build and dependency stack.

### Essential docs

- Build and dev setup: [CONTRIBUTING.md](CONTRIBUTING.md)
- Running a server: [docs/docs_running_a_server.md](docs/docs_running_a_server.md)
- Client installation: [docs/docs_client_installation.md](docs/docs_client_installation.md)
- Server config reference: [docs/docs_server_configuration_reference.md](docs/docs_server_configuration_reference.md)
- Ports and networking: [docs/docs_server_ports_usage.md](docs/docs_server_ports_usage.md)
- Repository structure: [docs/docs_repository_structure.md](docs/docs_repository_structure.md)

### Legal

- Terms of use: [TERMS.md](TERMS.md)
- Third-party licenses: [THIRD_PARTY_LICENSES](THIRD_PARTY_LICENSES)

## Русский

### Обзор

SkyMP добавляет мультиплеер в Skyrim. Этот репозиторий - центральная точка проекта для запуска сервера, сборки клиента и участия в разработке.

### Что можно делать в этом репозитории

1. Поднять собственный сервер.
2. Собрать клиент и сервер из исходников.
3. Помогать проекту кодом, документацией и тестами.

### Быстрый старт

#### Запуск сервера

Самый простой способ — скачать готовый релиз. **Клонировать репозиторий и собирать проект не нужно:**

1. Скачайте последний релиз с [github.com/skyrim-multiplayer/skymp/releases](https://github.com/skyrim-multiplayer/skymp/releases).
	- Windows: `running_server_files_windows_server_dist.zip`
	- Linux: `running_server_files_linux_server_dist.tar.gz`
2. Распакуйте архив в отдельную папку.
3. Запустите `launch_server.bat` (Windows) или `launch_server.sh` (Linux). Node.js и npm-зависимости установятся автоматически при первом запуске.
4. Отредактируйте `server-settings.json`: задайте имя сервера, порты и load order.
5. Откройте панель администратора: `http://<host>:<uiPort>/admin`.

Полная инструкция: [docs/docs_running_a_server.md](docs/docs_running_a_server.md).

#### Сборка из исходников (для разработчиков)

1. Следуйте [CONTRIBUTING.md](CONTRIBUTING.md).
2. Соберите проект.
3. Результаты лежат в `build/dist`.
4. Сервер: `build/dist/server`.
5. Клиент: `build/dist/client`.

#### Подключение игрока

1. Установите Skyrim SE или AE.
2. Установите SKSE по [docs/docs_client_installation.md](docs/docs_client_installation.md).
3. При локальной сборке скопируйте `build/dist/client` в папку Skyrim.
4. Запустите игру и подключитесь к серверу.

Если не хотите делать собственный лаунчер, можно использовать инструмент с Nexus:

https://www.nexusmods.com/skyrimspecialedition/mods/30379?tab=files

Параллельно мы работаем над глобальным лаунчером, чтобы владельцам серверов не приходилось делать и поддерживать отдельные собственные лаунчеры.

### Статус платформ

- Windows-сервер: стабильная и зрелая поддержка.
- Linux-сервер: лучший опыт на Ubuntu 24.04 и других дистрибутивах на базе glibc.
- Клиент: основной фокус по-прежнему на Windows.

### Ключевые разделы репозитория

- `skymp5-server`: сервер и admin API.
- `skymp5-front`: веб-панель администратора.
- `skymp5-client`: клиентская интеграция.
- `docs`: документация.
- `cmake`, `overlay_ports`, `overlay_triplets`, `vcpkg`: инфраструктура сборки и зависимостей.

### Важные документы

- Сборка и разработка: [CONTRIBUTING.md](CONTRIBUTING.md)
- Запуск сервера: [docs/docs_running_a_server.md](docs/docs_running_a_server.md)
- Установка клиента: [docs/docs_client_installation.md](docs/docs_client_installation.md)
- Настройки сервера: [docs/docs_server_configuration_reference.md](docs/docs_server_configuration_reference.md)
- Порты и сеть: [docs/docs_server_ports_usage.md](docs/docs_server_ports_usage.md)
- Структура репозитория: [docs/docs_repository_structure.md](docs/docs_repository_structure.md)

### Правовая информация

- Условия использования: [TERMS.md](TERMS.md)
- Сторонние лицензии: [THIRD_PARTY_LICENSES](THIRD_PARTY_LICENSES)

## Español

### Resumen

SkyMP lleva el modo multijugador a Skyrim. Este repositorio es el centro del proyecto para operar servidores, compilar componentes y colaborar en el desarrollo.

### Qué puedes hacer aquí

1. Alojar tu propio servidor.
2. Compilar cliente y servidor desde código fuente.
3. Contribuir con código, documentación y pruebas.

### Inicio rápido

#### Ejecutar un servidor

La forma más sencilla es descargar un paquete precompilado: **no necesitas clonar el repositorio ni compilar nada**.

1. Descarga el último release desde [github.com/skyrim-multiplayer/skymp/releases](https://github.com/skyrim-multiplayer/skymp/releases).
	- Windows: `running_server_files_windows_server_dist.zip`
	- Linux: `running_server_files_linux_server_dist.tar.gz`
2. Extrae el archivo en una carpeta dedicada.
3. Ejecuta `launch_server.bat` (Windows) o `launch_server.sh` (Linux). Node.js y las dependencias npm se instalan automáticamente en el primer inicio.
4. Edita `server-settings.json` con el nombre del servidor, puertos y orden de carga.
5. Abre el panel de administración en `http://<host>:<uiPort>/admin`.

Guía completa: [docs/docs_running_a_server.md](docs/docs_running_a_server.md).

#### Compilar desde el código fuente (para contribuidores)

1. Sigue [CONTRIBUTING.md](CONTRIBUTING.md).
2. Compila el proyecto.
3. La salida queda en `build/dist`.
4. Servidor: `build/dist/server`.
5. Cliente: `build/dist/client`.

#### Unirte como jugador

1. Instala Skyrim SE o AE.
2. Instala SKSE según [docs/docs_client_installation.md](docs/docs_client_installation.md).
3. Si compilas localmente, copia `build/dist/client` a tu carpeta de Skyrim.
4. Inicia el juego y conéctate a tu servidor.

Si no quieres crear tu propio launcher, esta herramienta de Nexus es una opción práctica:

https://www.nexusmods.com/skyrimspecialedition/mods/30379?tab=files

En paralelo, estamos trabajando en un launcher global para que los dueños de servidores no tengan que crear y mantener launchers personalizados por separado.

### Estado de plataforma

- Servidor Windows: soporte estable.
- Servidor Linux: la mejor experiencia es en Ubuntu 24.04 y otras distribuciones basadas en glibc.
- Cliente: enfoque principal todavía en Windows.

### Áreas clave del repositorio

- `skymp5-server`: servidor y APIs de administración.
- `skymp5-front`: frontend del panel de administración.
- `skymp5-client`: integración del lado cliente.
- `docs`: documentación completa.
- `cmake`, `overlay_ports`, `overlay_triplets`, `vcpkg`: infraestructura de build y dependencias.

### Documentos clave

- Configuración de build y desarrollo: [CONTRIBUTING.md](CONTRIBUTING.md)
- Ejecutar un servidor: [docs/docs_running_a_server.md](docs/docs_running_a_server.md)
- Instalación del cliente: [docs/docs_client_installation.md](docs/docs_client_installation.md)
- Configuración del servidor: [docs/docs_server_configuration_reference.md](docs/docs_server_configuration_reference.md)
- Puertos y red: [docs/docs_server_ports_usage.md](docs/docs_server_ports_usage.md)
- Estructura del repositorio: [docs/docs_repository_structure.md](docs/docs_repository_structure.md)

### Legal

- Términos de uso: [TERMS.md](TERMS.md)
- Licencias de terceros: [THIRD_PARTY_LICENSES](THIRD_PARTY_LICENSES)

## Français

### Vue d'ensemble

SkyMP ajoute le multijoueur a Skyrim. Ce depot est le point central pour executer des serveurs, compiler les composants et contribuer au projet.

### Ce que vous pouvez faire ici

1. Heberger votre propre serveur.
2. Compiler le client et le serveur depuis les sources.
3. Contribuer avec du code, de la documentation et des tests.

### Statut du projet (Fork WIP)

Ce fork est actuellement un projet **en cours de developpement**.

Certaines fonctionnalites sont deja utilisables, mais tout n'a pas encore ete teste sur toutes les configurations possibles. C'est aussi pour cela que ce fork n'est pas encore integre au depot principal.

Nous recherchons activement des testeurs qui peuvent:

1. Lancer leur propre serveur sur differents systemes.
2. Tester le gameplay et les flux d'administration en conditions reelles.
3. Signaler les bugs, crashs et problemes de configuration.

Comme chaque systeme est different, les retours de terrain sont tres importants.

### Demarrage rapide

#### Lancer un serveur

Le plus simple est de telecharger un build precompile: **pas besoin de cloner le depot ni de compiler**.

1. Telechargez la derniere release depuis [github.com/skyrim-multiplayer/skymp/releases](https://github.com/skyrim-multiplayer/skymp/releases).
	- Windows: `running_server_files_windows_server_dist.zip`
	- Linux: `running_server_files_linux_server_dist.tar.gz`
2. Extrayez l'archive dans un dossier dedie.
3. Lancez `launch_server.bat` (Windows) ou `launch_server.sh` (Linux). Node.js et les dependances npm sont installes automatiquement au premier lancement.
4. Modifiez `server-settings.json` (nom du serveur, ports, ordre de chargement).
5. Ouvrez le tableau de bord admin sur `http://<host>:<uiPort>/admin`.

Guide complet: [docs/docs_running_a_server.md](docs/docs_running_a_server.md).

#### Compiler depuis les sources (pour les contributeurs)

1. Suivez [CONTRIBUTING.md](CONTRIBUTING.md).
2. Compilez le projet.
3. Les artefacts sont dans `build/dist`.
4. Serveur: `build/dist/server`.
5. Client: `build/dist/client`.

#### Rejoindre en tant que joueur

1. Installez Skyrim SE ou AE.
2. Installez SKSE comme indique dans [docs/docs_client_installation.md](docs/docs_client_installation.md).
3. Si vous compilez localement, copiez `build/dist/client` dans votre dossier Skyrim.
4. Lancez le jeu et connectez-vous a votre serveur.

Si vous ne souhaitez pas creer votre propre launcher, cet outil Nexus est une option pratique:

https://www.nexusmods.com/skyrimspecialedition/mods/30379?tab=files

En parallele, nous travaillons sur un launcher global afin que les administrateurs de serveurs n'aient plus a maintenir des launchers personnalises separes.

### Statut plateforme

- Serveur Windows: support stable.
- Serveur Linux: meilleure experience sur Ubuntu 24.04 et distributions similaires basees sur glibc.
- Client: encore principalement axe Windows.

### Zones cle du depot

- `skymp5-server`: runtime serveur et API d'administration.
- `skymp5-front`: frontend du tableau de bord admin.
- `skymp5-client`: integration cote client.
- `docs`: documentation complete.
- `cmake`, `overlay_ports`, `overlay_triplets`, `vcpkg`: pile de build et dependances.

### Documents essentiels

- Setup build et developpement: [CONTRIBUTING.md](CONTRIBUTING.md)
- Lancer un serveur: [docs/docs_running_a_server.md](docs/docs_running_a_server.md)
- Installation client: [docs/docs_client_installation.md](docs/docs_client_installation.md)
- Reference de configuration serveur: [docs/docs_server_configuration_reference.md](docs/docs_server_configuration_reference.md)
- Ports et reseau: [docs/docs_server_ports_usage.md](docs/docs_server_ports_usage.md)
- Structure du depot: [docs/docs_repository_structure.md](docs/docs_repository_structure.md)

### Mentions legales

- Conditions d'utilisation: [TERMS.md](TERMS.md)
- Licences tierces: [THIRD_PARTY_LICENSES](THIRD_PARTY_LICENSES)

## Italiano

### Panoramica

SkyMP porta il multiplayer in Skyrim. Questo repository e il punto centrale per eseguire server, compilare i componenti e contribuire al progetto.

### Cosa puoi fare qui

1. Ospitare il tuo server.
2. Compilare client e server dai sorgenti.
3. Contribuire con codice, documentazione e test.

### Stato del progetto (Fork WIP)

Questo fork e attualmente un progetto **work in progress**.

Alcune funzionalita sono gia utilizzabili, ma non tutto e stato ancora testato su tutte le configurazioni possibili. Anche per questo motivo il fork non e ancora integrato nel repository principale.

Stiamo cercando attivamente tester che possano:

1. Avviare il proprio server su sistemi differenti.
2. Verificare gameplay e flussi admin in ambienti reali.
3. Segnalare bug, crash e problemi di configurazione.

Poiche ogni sistema e diverso, il feedback reale e molto importante.

### Avvio rapido

#### Avviare un server

Il modo piu semplice e scaricare un pacchetto precompilato: **non serve clonare il repository ne compilare**.

1. Scarica l'ultima release da [github.com/skyrim-multiplayer/skymp/releases](https://github.com/skyrim-multiplayer/skymp/releases).
	- Windows: `running_server_files_windows_server_dist.zip`
	- Linux: `running_server_files_linux_server_dist.tar.gz`
2. Estrai l'archivio in una cartella dedicata.
3. Esegui `launch_server.bat` (Windows) oppure `launch_server.sh` (Linux). Node.js e le dipendenze npm vengono installate automaticamente al primo avvio.
4. Modifica `server-settings.json` (nome server, porte, ordine di caricamento).
5. Apri la dashboard admin su `http://<host>:<uiPort>/admin`.

Guida completa: [docs/docs_running_a_server.md](docs/docs_running_a_server.md).

#### Compilare dai sorgenti (per contributor)

1. Segui [CONTRIBUTING.md](CONTRIBUTING.md).
2. Compila il progetto.
3. L'output e in `build/dist`.
4. Pacchetto server: `build/dist/server`.
5. Pacchetto client: `build/dist/client`.

#### Unirsi come giocatore

1. Installa Skyrim SE o AE.
2. Installa SKSE come descritto in [docs/docs_client_installation.md](docs/docs_client_installation.md).
3. Se compili in locale, copia `build/dist/client` nella cartella di Skyrim.
4. Avvia il gioco e collegati al tuo server.

Se non vuoi creare un tuo launcher, questo strumento su Nexus e un'opzione pratica:

https://www.nexusmods.com/skyrimspecialedition/mods/30379?tab=files

In parallelo, stiamo lavorando a un launcher globale cosi i gestori server non dovranno piu creare e mantenere launcher personalizzati separati.

### Stato piattaforme

- Server Windows: stabile e ben supportato.
- Server Linux: esperienza migliore su Ubuntu 24.04 e distribuzioni simili basate su glibc.
- Client runtime: ancora principalmente orientato a Windows.

### Aree chiave del repository

- `skymp5-server`: runtime server e API admin.
- `skymp5-front`: frontend dashboard admin.
- `skymp5-client`: integrazione lato client.
- `docs`: documentazione completa.
- `cmake`, `overlay_ports`, `overlay_triplets`, `vcpkg`: stack di build e dipendenze.

### Documenti essenziali

- Setup build e sviluppo: [CONTRIBUTING.md](CONTRIBUTING.md)
- Eseguire un server: [docs/docs_running_a_server.md](docs/docs_running_a_server.md)
- Installazione client: [docs/docs_client_installation.md](docs/docs_client_installation.md)
- Riferimento configurazione server: [docs/docs_server_configuration_reference.md](docs/docs_server_configuration_reference.md)
- Porte e rete: [docs/docs_server_ports_usage.md](docs/docs_server_ports_usage.md)
- Struttura repository: [docs/docs_repository_structure.md](docs/docs_repository_structure.md)

### Legale

- Termini di utilizzo: [TERMS.md](TERMS.md)
- Licenze di terze parti: [THIRD_PARTY_LICENSES](THIRD_PARTY_LICENSES)
