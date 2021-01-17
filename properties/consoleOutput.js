module.exports = () => {
  const consoleOutput = {};

  const printTargets = {
    consoleOutput: "ctx.sp.printConsole(...ctx.value.args)",
    notification: "ctx.sp.Debug.notification(...ctx.value.args)",
  };

  for (const propName of Object.keys(printTargets)) {
    const updateOwner = () => `
      if (ctx.state.n${propName} === ctx.value.n) return;
      ctx.state.n${propName} = ctx.value.n;
      if (ctx.value.date) {
        if (Math.abs(ctx.value.date - +Date.now()) > 2000) return;
      }
      ${printTargets[propName]}
    `;
    mp.makeProperty(propName, {
      isVisibleByOwner: true,
      isVisibleByNeighbors: false,
      updateNeighbor: "",
      updateOwner: updateOwner(),
    });
  }

  const genericPrint = (propName, formId, ...printConsoleArgs) => {
    const prev = mp.get(formId, propName);
    const n = prev ? prev.n : 0;
    mp.set(formId, propName, {
      n: n + 1,
      args: printConsoleArgs,
      date: +Date.now(),
    });
  };

  consoleOutput.print = (...args) => genericPrint("consoleOutput", ...args);
  consoleOutput.printNote = (...args) => genericPrint("notification", ...args);

  return consoleOutput;
};
