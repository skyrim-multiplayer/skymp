Hello!

If this folder is empty for you minus a few .bat file and this read me, congrats! You've done nothing wrong. 

If this folder is full of server files, like a server-settings.json or a data folder, OH NO! WE DID SOMETHING WRONG!
Unless of course, you put those files there yourself after running a build, in which case, CONGRATS! ALL IS WELL!



=== DO NOT CLICK ON ANY BAT FILES!! ===

The first time you get this folder, check the install-services.bat file (right click, edt) to make sure the pathing is correct. 
By default, its expected to run at C:\Users\Administrator\Desktop\SkyMP\build\dist\server
But, if you have a different path, you'll want to update that. Once its all good, you're free to run it by clicking on it. 

After NSSM is installed, your server will now be a service, which means it'll run automatically when your computer starts, or if the program crashes. 
You want this if you're running a server with a lot of players. After its installed, you're free to delete that bat.

If there are any errors, it should also spit out log files onto your desktop (or anywhere else if you changed it).





Next we have the restart-server.bat file. 
You will need to run this whenever you make changes to the server files.
If you update the settings, pull the repo to update your files, or install an updated build of the server, run this bat. 

I've also added stop-server.bat and start-server.bat
If you need to kill the server or manually restart it, these two are your buttons.

To make your life easier, just move the shortcuts located in "Desktop_Shortcuts" onto your desktop. 
Now you have 3 easy to run scripts to manage your server! Yay!





Finally, we wanna look at the server-settings.json file. 
What we start with is pretty barebones, so here's an example below you can fill out yourself

Once all of this is done, CONGRATS! Now all we have to do is configure the backend. 


{
  "dataDir": "data",
  "gamemode": "gamemode.js",
  
  "loadOrder": [
    "C:/GOG Games/Skyrim Anniversary Edition/Data/Skyrim.esm",
    "C:/GOG Games/Skyrim Anniversary Edition/Data/Update.esm",
    "C:/GOG Games/Skyrim Anniversary Edition/Data/Dawnguard.esm",
    "C:/GOG Games/Skyrim Anniversary Edition/Data/HearthFires.esm",
    "C:/GOG Games/Skyrim Anniversary Edition/Data/Dragonborn.esm"
  ],
  "archives": ["Skyrim - Misc.bsa"],
  
  "name": "name here",
  "port": 7777,
  "maxPlayers": 800,
  "offlineMode": false,
  
  "master": "https://api.yourwebsitehere.com",
  "masterKey": "same as .env",
  "masterApiAuthToken": "same as .env",
  
  "npcEnabled": false,
  "npcSettings": {},
  
  "isPapyrusHotReloadEnabled": true,
  "enableGamemodeDataUpdatesBroadcast": true,
  "enableConsoleCommandsForAll": false,
  
  "metricsAuth": {
    "user": "same as .env",
    "password": "same as .env"
  },
  
  "damageMultFormulaSettings": { "multiplier": 1.0 },
  
  "discordAuth": {
    "botToken": "insert token here",
    "guildId": "insert ID here",
    "banRoleId": "insert ID here"
  }
}
