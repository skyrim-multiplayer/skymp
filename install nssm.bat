nssm install SkyrpGameServer "C:\Program Files\nodejs\node.exe" "dist_back\skymp5-server.js"
nssm set SkyrpGameServer AppDirectory "C:\Users\Administrator\Desktop\SkyMP\build\dist\server"
nssm set SkyrpGameServer AppStdout "C:\logs\gameserver.log"
nssm set SkyrpGameServer AppStderr "C:\logs\gameserver-err.log"
nssm start SkyrpGameServer