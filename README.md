# scamp-server

Install:
`npm install`

Enable copying `MpClientPlugin.dll` to your Skyrim Special Edition Installation:
`npm run configure -- --CDSKYRIM_DIR="..."`

Enable coverage calculation for C++ code (Windows-only, OpenCppCoverage must be installed in your system):
`npm run configure -- --CDCPPCOV=ON`

Run tests:
`npm run test`
