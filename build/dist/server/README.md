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





Next we have the restart-server.bat file. This is located in \SkyMP\deploy\
You will need to run this whenever you make changes to the server files. 
If you update the settings, pull the repo to update your files, or install an updated build of the server, run this bat. 

You may also need to grab a new dist if you update certain files, JUST MAKE SURE TO BACKUP YOUR SERVER-SETTINGS.JSON AND YOUR GAMEMODE.JS!!!

I've also added stop-server.bat and start-server.bat
If you need to kill the server or manually restart it, these two are your buttons.

To make your life easier, just move the shortcuts located in "Desktop_Shortcuts" onto your desktop. 
Now you have 3 easy to run scripts to manage your server! Yay!





Finally, we wanna look at the server-settings.json file. 
Once all of this is done, CONGRATS! Now all we have to do is configure the backend. 

What we start with is pretty barebones, so here's an example below you can fill out yourself
The default starting point is whiterun (you can delete it, and it just drops you on a cliff near Riften)

The reloot is what items respawn, its in milliseconds. By default, everything is an hour, but here I've set harvestables to 15min, and containers to 24hr.
Cont is containers (barrels, chests, etc), flor/tree are harvestables, and everything else is loose items which I put to never respawn. 

There's also settings for database, logging, sweetpie settings, language, etc you can find in the documentation wiki. 
For now, this should be all you need.

{
  "dataDir": "data",
  "gamemodePath": "gamemode.js",
  
  "loadOrder": [
    "C:/Steam/steamapps/common/Skyrim Special Edition/Data/Skyrim.esm",
    "C:/Steam/steamapps/common/Skyrim Special Edition/Data/Update.esm",
    "C:/Steam/steamapps/common/Skyrim Special Edition/Data/Dawnguard.esm",
    "C:/Steam/steamapps/common/Skyrim Special Edition/Data/HearthFires.esm",
    "C:/Steam/steamapps/common/Skyrim Special Edition/Data/Dragonborn.esm"
  ],
  "archives": ["C:/Steam/steamapps/common/Skyrim Special Edition/Data/Skyrim - Misc.bsa"],
  
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
  "enableGamemodeDataUpdatesBroadcast": false,
  "enableConsoleCommandsForAll": false,
  
  "metricsAuth": {
    "user": "same as .env",
    "password": "same as .env"
  },
  
  "damageMultFormulaSettings": { "multiplier": 1.0 },

  "startPoints": [
    {
      "pos": [22659, -8697, -3594],
      "worldOrCell": "0x1a26f",
      "angleZ": 268
    }
  ],

  "reloot": {
    "FLOR": 900000,
    "TREE": 900000,
    "CONT": 86400000
  },
  
  "forbiddenReloot": ["MISC", "WEAP", "SLGM", "SCRL", "ALCH", "INGR", "BOOK", "ARMO", "AMMO"]
    
  "discordAuth": {
    "botToken": "[insert token here]",
    "guilds": [
      {
        "guildId": "[insert server ID here]",
        "banRoleId": "[insert role ID here]"
        "eventLogChannelId": "[insert channel ID here]",
        "hideIpRoleId": "[insert admin ID here]"
      }
    ]
  }
}
