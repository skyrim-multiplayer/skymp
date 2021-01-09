# Database Drivers

Skyrim Multiplayer mimics the original game behavior, so unlike many multiplayer mods in other games, it stores world and players data automatically.

This page describes the differences between database drivers and their functionality.

## sqlite

Skyrim Multiplayer uses `sqlite` driver by default. Default `databaseName` is `world.sqlite`. So if you change nothing in the config file, the server would save all data using SQLite 3 in the world.sqlite server in the server directory. Only relative paths are supported.

```json
{
  // ...
  "databaseDriver": "sqlite",
  "databaseName": "world.sqlite"
  // ...
}
```

## mongodb

Uses MongoDB to store data. Built for servers targeting real-world players from the Internet, not testers or a couple of your friends you play in coop with.

`databaseUri` is a mongo URI used to connect to a remote MongoDB instance.

We recommend [MongoDB Atlas](https://www.mongodb.com/cloud/atlas) as a cloud database service.

```json
{
  // ...
  "databaseDriver": "mongodb",
  "databaseName": "db",
  "databaseUri": "mongodb://<user>:<pass>@cluster0-shard-00-00.xxxxx.mongodb.net:27017,cluster0-shard-00-01.xxxxx.mongodb.net:27017,cluster0-shard-00-02.xxxxx.mongodb.net:27017/<dbname>?ssl=true&replicaSet=atlas-a16dc0-shard-0&authSource=admin&retryWrites=true&w=majority"
  // ...
}
```

## migration

A special database driver is used to move from one type of database to another on the fly. Do not forget to backup everything before using this.

```json
{
  // ...
  "databaseDriver": "migration",
  "databaseOld": {
    "databaseDriver": "sqlite",
    "databaseName": "world.sqlite"
  },
  "databaseNew": {
    "databaseDriver": "mongodb"
    // ...
  }
  // ...
}
```
