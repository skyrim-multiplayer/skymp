const path = require("path");
const fs = require("fs");
const utils = require("./utility/utils");
const consoleOutput = require("./properties/consoleOutput");

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

const traceAnims = false;
if (traceAnims) {
  mp.makeEventSource(
    "_onAnimationEvent",
    `
        const next = ctx.sp.storage._api_onAnimationEvent;
        ctx.sp.storage._api_onAnimationEvent = {
          callback(...args) {
            const [serversideFormId, animEventName] = args;
            ctx.sendEvent(serversideFormId, animEventName);
            if (typeof next.callback === "function") {
              next.callback(...args);
            }
          }
        };
      `
  );

  modulesLoaded.utils.hook(
    "_onAnimationEvent",
    (pcFormId, serversideFormId, animEventName) => {
      if (serversideFormId !== 0x14) return;
      modulesLoaded.consoleOutput.print(pcFormId, animEventName);
    }
  );
}

mp.makeEventSource(
  "_onActivate",
  `
  ctx.sp.on("activate", (e) => {
    const target = ctx.getFormIdInServerFormat(e.target.getFormId());
    const caster = ctx.getFormIdInServerFormat(e.caster.getFormId());
    if (caster !== 0x14) return;
    ctx.sendEvent({
      target
    });
  });
`
);

modulesLoaded.utils.hook("_onActivate", (pcFormId, event) => {
  modulesLoaded.consoleOutput.print(pcFormId, event);

  if (event.target === 0x74ee2) {
    /*modulesLoaded.consoleOutput.evalClient(
      pcFormId,
      "ctx.sp.Debug.SendAnimationEvent(ctx.sp.Game.getPlayer(), 'jumpstandingstart')"
    );*/
    // ...
  }
});
