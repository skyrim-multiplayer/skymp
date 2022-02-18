Added `error` field to `HttpResponse`. This allows checking for more detailed info
why your HTTP request failed. Currently, it can be on of these:
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
