# scamp-server

Install:
`npm install`

Enable copying `MpClientPlugin.dll` to your Skyrim Special Edition Installation:
`npm run configure -- --CDSKYRIM_DIR="..."`

Enable coverage calculation for C++ code (Windows-only, OpenCppCoverage must be installed in your system):
`npm run configure -- --CDCPPCOV=ON`

Run tests:
`npm run test`

Debugging with GDB (Linux):

1. Find out what pid is used by node (usually it is 24)
   `docker exec -ti <container_name> ps -eaf`
2. Attach
   `docker exec -ti --privileged <container_name> gdb -p <node_pid>`

Debugging unit tests (Linux):

1. Start debugging `docker run -it <image_name> gdb build/scamp_native/unit`
2. Type `r` to run

Useful links (RU):
- https://eax.me/gdb/
