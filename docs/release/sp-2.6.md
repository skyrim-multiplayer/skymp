# SP 2.6.0 Release Notes


This document includes changes made since SP 2.5.0


SP updates regularly. This update probably doesn't include ALL patches that have to be made.

There are still many things to be implemented or fixed. See [issues](https://github.com/skyrim-multiplayer/skymp/issues?q=is%3Aopen+is%3Aissue+label%3Aarea%3Askyrim-platform).

SP supports both Steam version of Skyrim SE (1.6+) and legacy 1.5.97.

Fixed displaying of non-ASCII text: fixed working with UTF-8 (from JS) text and recompiled font to include Cyrillic symbols. In case you wish to update the font to use something else, please see [SP Texts API docs](../texts.md#compiling-font).


## `error` in HTTP client's response

Added `error` field to `HttpResponse`. This allows checking for more detailed info
why your HTTP request failed. Currently, it can be
[one of these](https://github.com/yhirose/cpp-httplib/blob/b80aa7fee31a8712b1d3cae05c1d9e7f5c436e3d/httplib.h#L771-L785):
* Empty string, if successful
* `Unknown`
* `Connection`
* `BindIPAddress`
* `Read`
* `Write`
* `ExceedRedirectCount`
* `Canceled`
* `SSLConnection`
* `SSLLoadingCerts`
* `SSLServerVerification`
* `UnsupportedMultipartBoundaryChars`
* `Compression`


## FileInfo API

Now you can get info about files directly inside of Skyrim's Data directory.

```ts
sp.printConsole(JSON.stringify(sp.getFileInfo('Skyrim.esm')));
// {"filename": "Skyrim.esm", "size": ..., "crc32": ...}
```


## Other changes

- Fixed a few security issues related to local file paths checking.


- Skyrim Platform now have settings file located at `Data\SKSE\Plugins\SkyrimPlatform.ini`.


- The project now complies with C++20 and compiles with VS 2022 without errors.


- CommonlibSSE updated to latest version.


- Runtime event sink handling introduced. Except for some custom events that fire from within the code, events now will not be processed if there are no plugins that are subscribed to them. In theory this should provide more execution performance since there are less calls that need to be processed.


- Logging introduced. Platform now uses skse runtime logging capabilities to provide users more feedback about its execution succession. Log file is located at `User\Documents\My Games\Skyrim Special Edition\SKSE.`


- All raw offsets for hooks and game classes were replaced with Address Library IDs to support future game version changes.


- SKSE64 is no longer a build dependency (Still required to run the plugin).
