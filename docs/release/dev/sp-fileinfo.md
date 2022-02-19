## FileInfo API

Now you can get info about files directly inside of Skyrim's Data directory.

```ts
sp.printConsole(JSON.stringify(sp.getFileInfo('Skyrim.esm')));
// {"filename": "Skyrim.esm", "size": ..., "crc32": ...}
```
