let fs = require('fs');
let path = require('path');
let childProcess = require('child_process');
let game = require('./game');

function writeFileSyncRecursive(filename, content, charset) {
	filename.split(path.sep).slice(0,-1).reduce( (last, folder)=>{
		let folderPath = last ? (last + path.sep + folder) : folder
		if (!fs.existsSync(folderPath)) fs.mkdirSync(folderPath)
		return folderPath
	})
	
	fs.writeFileSync(filename, content, charset)
}

if (!fs.existsSync('config.js')) {
  fs.writeFileSync('config.js', fs.readFileSync('config-default.js'));
}
let config = require('./config');

console.log("Dev service started");

let bin = path.join(config.SkyrimMultiplayerBinaryDir,
  '_platform_se');
fs.watch(bin, (eventType, fileName) => {
  if (fileName === 'touch_Release' || fileName === 'touch_Debug') {
    let buildCfg = fileName === 'touch_Release' ? 'Release' : 'Debug';

    console.log('Skyrim Platform x64 updated. Exiting game');

    game.kill();

    let pluginName = 'SkyrimPlatform.dll';
    for (file of ['SkyrimPlatform.pdb', 'ChakraCore.dll', 'SkyrimPlatform.dll']) {
      console.log(`Updating ${file}`);
      from = path.join(bin, `bin/${buildCfg}/${file}`);
      to = pluginName === file ?
        path.join(config.SkyrimSEFolder, 'Data/SKSE/Plugins', file) :
        path.join(config.SkyrimSEFolder, file);
        writeFileSyncRecursive(to, fs.readFileSync(from));
    }

    console.log('Restarting');
    game.launch(config.SkyrimSEFolder).catch(e => console.error(e));
  }
});
