Hello!

If this folder is empty for you minus a few .bat file and this read me, congrats! You've done nothing wrong. 

If this folder is full of server files, like a server-settings.json or a data folder, OH NO! WE DID SOMETHING WRONG!
Unless of course, you put those files there yourself after running a build, in which case, CONGRATS! ALL IS WELL!


The first time you get this folder, check the install-services.bat file (right click, edt) to make sure the pathing is correct. 
By default, its expected to run at C:\Users\Administrator\Desktop\SkyMP\build\dist\server
But, if you have a different path, you'll want to update that. 

After NSSM is installed, your server will now be a service, which means it'll run automatically when your computer starts, or if the program crashes. 
You want this if you're running a server with a lot of players. 
After its installed, you're free to delete that bat.


Next we have the restart-server.bat file. 
You will need to run this whenever you make changes to the server files.
If you update the settings, pull the repo to update your files, or install an updated build of the server, run this bat. 

I've also added stop-server.bat and start-server.bat
If you need to kill the server or manually restart it, these two are your buttons.

=== I recommend shortcutting all three of these bat files, and placing them on your desktop for ease of use. ===

Finally, we wanna look at the server-settings.json file. 
What we start with is pretty barebones, so here's a list of everything you might want:

