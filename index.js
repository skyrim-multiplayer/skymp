const path = require("path");
const fs = require("fs");
const { Module } = require("module");

const requireUncached = (module) => {
  delete require.cache[require.resolve(module)];
  return require(module);
};

const { gamemodePath } = JSON.parse(fs.readFileSync("server-settings.json"));

const toAbsolute = (p) =>
  path.isAbsolute(p) ? p : path.resolve(gamemodePath, p);

const modulesLoaded = {};

[
  "utility/utils",
  "utility/typecheck",
  "events/onHit",
  "properties/isDead",
  "events/onSprintStateChange",
  "events/onPowerAttack",
  "events/onBash",
  "properties/consoleOutput",
  "properties/actorValues",
  "events/onActorValueFlushRequired",
  "properties/spawnSystem",
  "events/onConsoleCommand",
  "systems/developerCommands",
].forEach((moduleName) => {
  const moduleInitFunction = requireUncached(toAbsolute(moduleName));
  const module = moduleInitFunction(modulesLoaded);

  const moduleShortName = moduleName.split("/").pop();
  modulesLoaded[moduleShortName] = module;
});

modulesLoaded.utils.hook("onInit", (pcFormId) => {
  mp.onReinit(pcFormId);
});
