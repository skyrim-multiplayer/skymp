module.exports = (modules) => {
  const { utils, actorValues } = modules;

  for (const attr of ["Health", "Magicka", "Stamina"]) {
    mp.makeEventSource(
      "_onActorValueFlushRequired" + attr,
      `
        const update = () => {
          const attr = "${attr}";
          const percent = ctx.sp.Game.getPlayer().getActorValuePercentage(attr);
          if (ctx.state.percent !== percent) {
            if (ctx.state.percent !== undefined && percent === 1) {
              ctx.sendEvent();
            }
            ctx.state.percent = percent;
          }
        };
        (async () => {
          while (1) {
            await ctx.sp.Utility.wait(0.667);
            update();
          }
        })();
      `
    );
    utils.hook("_onActorValueFlushRequired" + attr, (pcFormId) => {
      actorValues.flushRegen(pcFormId, attr);
    });
  }
};
