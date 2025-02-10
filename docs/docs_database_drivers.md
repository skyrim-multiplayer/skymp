# Database Drivers

Skyrim Multiplayer mimics the original game behavior, so unlike many multiplayer mods in other games, it stores world and players data automatically.

This page describes the differences between database drivers and their functionality.

## file

Skyrim Multiplayer uses `file` driver by default. Default `databaseName` is `world`. So if you change nothing in the config file, the server would save all data using `world` subdirectory of the server folder. Only relative paths are supported.

```json5
{
  // ...
  "databaseDriver": "file",
  "databaseName": "world"
  // ...
}
```

## mongodb

Uses MongoDB to store data. Built for servers targeting real-world players from the Internet, not testers or a couple of your friends you play in coop with.

`databaseUri` is a mongo URI used to connect to a remote MongoDB instance.

We recommend [MongoDB Atlas](https://www.mongodb.com/cloud/atlas) as a cloud database service.

```json5
{
  // ...
  "databaseDriver": "mongodb",
  "databaseName": "db",
  "databaseUri": "mongodb://<user>:<pass>@cluster0-shard-00-00.xxxxx.mongodb.net:27017,cluster0-shard-00-01.xxxxx.mongodb.net:27017,cluster0-shard-00-02.xxxxx.mongodb.net:27017/<dbname>?ssl=true&replicaSet=atlas-a16dc0-shard-0&authSource=admin&retryWrites=true&w=majority"
  // ...
}
```

## zip

Similar to `file` driver, but uses zip archive instead of directory. Default `databaseName` is `world`. The server would use `world.zip` for data storage in this case.

```json5
{
  // ...
  "databaseDriver": "zip",
  "databaseName": "world"
  // ...
}
```

## migration

A special database driver is used to move from one type of database to another on the fly. Do not forget to backup everything before using this.

```json5
{
  // ...
  "databaseDriver": "migration",
  "databaseOld": {
    "databaseDriver": "file",
    "databaseName": "world"
  },
  "databaseNew": {
    "databaseDriver": "mongodb"
    // ...
  }
  // ...
}
```
