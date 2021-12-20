import { hooks, Game, printConsole, Actor } from "skyrimPlatform";

let gAllowGetUp = true;

export const update = (): void => {
  gAllowGetUp =
    (Game.getPlayer() as Actor).getActorValuePercentage("health") >= 0.05;
};

hooks.sendAnimationEvent.add({
  enter(ctx) {
    if (ctx.animEventName.toLowerCase().includes("killmove")) {
      ctx.animEventName = "";
    }

    if (ctx.selfId !== 0x14) return;
    if (!gAllowGetUp && ctx.animEventName === "GetUpBegin") {
      ctx.animEventName = "";
      printConsole("block GetUpBegin");
    }
  },
  leave() {
    return;
  },
});

export const makeActorImmortal = (act: Actor): void => {
  act.startDeferredKill();
};
