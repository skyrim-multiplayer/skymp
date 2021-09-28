# Server Command Line API

During the very first months of the current generation of skymp server's existence, we used command line API to configure our servers. These days we have [a configuration file](docs_server_configuration_reference.md) in JSON format, but configuring via command-line arguments is also supported.

```bash
--maxPlayers 108 --name "Server X" --port 7777 --ip "127.0.0.1" --offlineMode false
```

Things you need to know about command-line arguments before using one:

- Arguments passed directly to a command line implicitly override values from `server-settings.json` if they are set.
- The only options that exist as command-line arguments are `maxPlayers`, `name`, `port`, `ip`, `offlineMode`. They have the same meanings as options in `server-settings.json`.
- Instead of writing `--maxPlayers 108` you may use `-m 108`. Only `maxPlayers` option has a short version.
