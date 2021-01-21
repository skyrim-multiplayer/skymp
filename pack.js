const fs = require("fs");
const path = require("path");

const makeDirectory = (p) => {
  if (!fs.existsSync(p)) {
    fs.mkdirSync(p);
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

  const gm = "gamemode.js";
  fs.writeFileSync(path.join(packPath, gm), "/* TODO: Add gamemode */");

  makeDirectory(path.join(packPath, "dist_front"));
  const client = "dist_front/skymp5-client.js";
  fs.writeFileSync(path.join(packPath, client), fs.readFileSync(client));

  makeDirectory(path.join(packPath, "data"));
  makeDirectory(path.join(packPath, "data/scripts"));
};

if (process.platform === "win32") {
  packWin32();
} else {
  throw new Error(`pack.js - Unsupported platform '${process.platform}'`);
}
