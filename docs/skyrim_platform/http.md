# HTTP requests

SkyrimPlatform provides limited support for HTTP/HTTPS requests.
At the moment only `get` and `post` are available.

```typescript
import { HttpClient } from "skyrimPlatform";
let url = "https://canhazip.com:443"; // URL may contain port or not
let http = new HttpClient(url);
http.get("/").then((response) => printConsole(response.body));
```

- In case the request fails, `response.body` will be empty.
