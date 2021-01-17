module.exports = () => {
  mp.makeEventSource(
    "_onSprintStateChange",
    `
        ctx.sp.on("update", () => {
          const isSprinting = ctx.sp.Game.getPlayer().isSprinting();
          if (ctx.state.isSprinting !== isSprinting) {
            if (ctx.state.isSprinting !== undefined) {
              ctx.sendEvent(isSprinting ? "start" : "stop");
            }
            ctx.state.isSprinting = isSprinting;
          }
        });
      `
  );
};
