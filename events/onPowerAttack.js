module.exports = () => {
  mp.makeEventSource(
    "_onPowerAttack",
    `
        const next = ctx.sp.storage._api_onAnimationEvent;
        ctx.sp.storage._api_onAnimationEvent = {
          callback(...args) {
            const [serversideFormId, animEventName] = args;
            if (serversideFormId === 0x14 && animEventName.toLowerCase().includes("power")) {
              ctx.sendEvent(serversideFormId);
            }
            if (typeof next.callback === "function") {
              next.callback(...args);
            }
          }
        };
      `
  );
};
