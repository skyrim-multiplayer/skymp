module.exports = (modules) => {
  const { spawnSystem, utils, consoleOutput } = modules;

  const chooseFormId = (pcFormId, selectedFormId) => {
    return selectedFormId ? selectedFormId : pcFormId;
  };

  const chooseTip = (pcFormId, selectedFormId) => {
    return selectedFormId ? "(selected)" : "(your character)";
  };

  const reinit = (pcFormId, selectedFormId) => {
    const targetFormId = chooseFormId(pcFormId, selectedFormId);
    const tip = chooseTip(pcFormId, selectedFormId);

    //actorValues.setDefaults(targetFormId, { force: true });
    mp.onReinit(targetFormId, { force: true });

    consoleOutput.print(
      targetFormId,
      `Reinit ${targetFormId.toString(16)} ${tip}`
    );
  };

  const setav = (pcFormId, selectedFormId, avName, newValueStr) => {
    let newValue = parseFloat(newValueStr);
    newValue = isFinite(newValue) ? newValue : 1;

    const targetFormId = chooseFormId(pcFormId, selectedFormId);
    const tip = chooseTip(pcFormId, selectedFormId);
    actorValues.set(targetFormId, avName, "base", newValue);
    consoleOutput.print(targetFormId, `Set ${avName} to ${newValue} ${tip}`);
  };

  const kill = (pcFormId, selectedFormId) => {
    const targetFormId = chooseFormId(pcFormId, selectedFormId);
    const tip = chooseTip(pcFormId, selectedFormId);

    const prev = mp.get(targetFormId, "isDead");
    mp.set(targetFormId, "isDead", !prev);

    consoleOutput.print(
      targetFormId,
      "Play visual effects for killing/resurrection",
      `${targetFormId.toString(16)} ${tip}`
    );
  };

  const spawn = (pcFormId, selectedFormId) => {
    const targetFormId = chooseFormId(pcFormId, selectedFormId);
    const tip = chooseTip(pcFormId, selectedFormId);

    spawnSystem.spawn(targetFormId);

    consoleOutput.print(
      targetFormId,
      `Teleporting to the spawnpoint ${targetFormId.toString(16)} ${tip}`
    );
  };

  const spawnpoint = (pcFormId, selectedFormId) => {
    const targetFormId = chooseFormId(pcFormId, selectedFormId);
    const tip = chooseTip(pcFormId, selectedFormId);

    spawnSystem.updateSpawnPoint(targetFormId);

    consoleOutput.print(
      targetFormId,
      `Spawnpoint has been updated for ${targetFormId.toString(16)} ${tip}`
    );
  };

  utils.hook("_onConsoleCommand", (pcFormId, ...args) => {
    const selectedFormId = args[0] !== 0x14 ? args[0] : pcFormId;
    const sub = args[1];
    const arg0 = args[2];
    const arg1 = args[3];
    if (sub === "reinit") {
      reinit(pcFormId, selectedFormId);
    } else if (sub === "setav") {
      setav(pcFormId, selectedFormId, arg0, arg1);
    } else if (sub === "kill") {
      kill(pcFormId, selectedFormId);
    } else if (sub === "spawn") {
      spawn(pcFormId, selectedFormId);
    } else if (sub === "spawnpoint") {
      spawnpoint(pcFormId, selectedFormId);
    }
  });
};
