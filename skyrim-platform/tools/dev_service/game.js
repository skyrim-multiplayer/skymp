let path = require('path');
let fs = require('fs');
let childProcess = require('child_process');

module.exports = {
  kill() {
    while (true) {
      let tasklist = childProcess.execSync('tasklist').toString('utf8');
      let appName = 'SkyrimSE.exe';
      let gameStillRunning = tasklist.indexOf(appName) !== -1;

      if (!gameStillRunning) break;
      try {
        childProcess.execSync(`taskkill /im ${appName} /f`);
      }
      catch(e) {
        // ...
      }
    }
  },

  launch(folder) {
    return new Promise((resolve, reject) => {
      let tmpFile = path.join(
        path.dirname(process.env.APPDATA), 'Local/Temp/launch_skse64.bat');
      fs.writeFileSync(tmpFile, 'cd ' + folder + ' && ' + 'skse64_loader.exe');

      const p = childProcess.spawn(tmpFile, []);
      p.stderr.on('data', async data => {
        reject(new Error(`skse64_loader failed, stderr: ${data}`));
      });
      p.stdout.on('data', () => resolve());
    });
  }
};
