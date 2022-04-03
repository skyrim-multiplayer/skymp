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
