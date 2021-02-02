const fs = require("fs");
const path = require("path");

const makeDirectory = (p) => {
  if (!fs.existsSync(p)) {
    fs.mkdirSync(p);
  }
};

// https://stackoverflow.com/questions/13786160/copy-folder-recursively-in-node-js
/**
 * Look ma, it's cp -R.
 * @param {string} src  The path to the thing to copy.
 * @param {string} dest The path to the new copy.
 */
var copyRecursiveSync = function (src, dest) {
  var exists = fs.existsSync(src);
  var stats = exists && fs.statSync(src);
  var isDirectory = exists && stats.isDirectory();
  if (isDirectory) {
    makeDirectory(dest); // fix
    fs.readdirSync(src).forEach(function (childItemName) {
      copyRecursiveSync(
        path.join(src, childItemName),
        path.join(dest, childItemName)
      );
    });
  } else {
    fs.copyFileSync(src, dest);
  }
};

const packWin32 = () => {
  const packPath = "./pack";
  makeDirectory(packPath);

  const exe = "skymp5-server.exe";
  fs.writeFileSync(path.join(packPath, exe), fs.readFileSync(exe));

  const cfg = "server-settings.json";
  const defaultCfg = {
    dataDir: "data",
    loadOrder: [
      "Skyrim.esm",
      "Update.esm",
      "Dawnguard.esm",
      "HearthFires.esm",
      "Dragonborn.esm",
    ],
    ip: "127.0.0.1",
    name: "My Server",
  };
  fs.writeFileSync(
    path.join(packPath, cfg),
    JSON.stringify(defaultCfg, null, 4)
      .split("")
      .map((x) => (x === "\n" ? "\r\n" : x))
      .join(""),
    {
      encoding: "utf-8",
    }
  );

  const addon = "scamp_native.node";
  fs.writeFileSync(
    path.join(packPath, addon),
    fs.readFileSync(path.join("build/Release", addon))
  );

  // Required by client but builds with the server
  const mpClientPlugin = "MpClientPlugin.dll";
  fs.writeFileSync(
    path.join(packPath, mpClientPlugin),
    fs.readFileSync(path.join("build/Release", mpClientPlugin))
  );

  const gm = "gamemode.js";
  fs.writeFileSync(path.join(packPath, gm), "/* TODO: Add gamemode */");

  makeDirectory(path.join(packPath, "dist_front"));
  const client = "dist_front/skymp5-client.js";
  fs.writeFileSync(path.join(packPath, client), fs.readFileSync(client));

  makeDirectory(path.join(packPath, "data"));
  makeDirectory(path.join(packPath, "data/scripts"));

  copyRecursiveSync("./ui", path.join(packPath, "data/ui"));

  const libkey = "data/_libkey.js";
  fs.writeFileSync(path.join(packPath, libkey), fs.readFileSync(libkey));

  const packageJson = JSON.parse(
    fs.readFileSync("package.json", { encoding: "utf-8" })
  );

  const readme = "README";
  let readmeContent = fs.readFileSync(readme, "utf-8");
  readmeContent = readmeContent.replace("!!DATE!!", new Date());

  const projectVersionTag = require("child_process")
    .execSync("git describe --tags")
    .toString()
    .trim();
  readmeContent = readmeContent.replace(
    "!!PROJECT_VERSION!!",
    projectVersionTag
  );

  const clientRevision = require("child_process")
    .execSync("git rev-parse HEAD", { cwd: "./skymp5-client" })
    .toString()
    .trim();
  readmeContent = readmeContent.replace("!!CLIENT_VERSION!!", clientRevision);

  const gamemodeRevision = require("child_process")
    .execSync("git rev-parse HEAD", { cwd: "./skymp5-gamemode" })
    .toString()
    .trim();
  readmeContent = readmeContent.replace(
    "!!GAMEMODE_VERSION!!",
    gamemodeRevision
  );

  readmeContent = readmeContent.replace(
    "!!SKYRIM_PLATFORM_VERSION!!",
    packageJson.versionSkyrimPlatform
  );
  fs.writeFileSync(path.join(packPath, readme), readmeContent);
};

if (process.platform === "win32") {
  packWin32();
} else {
  throw new Error(`pack.js - Unsupported platform '${process.platform}'`);
}
