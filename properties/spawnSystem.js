module.exports = (modules) => {
  const { utils, actorValues } = modules;

  const spawnSystem = {};

  mp.makeProperty("spawnPoint", {
    isVisibleByOwner: false,
    isVisibleByNeighbors: false,
    updateNeighbor: "",
    updateOwner: "",
  });

  const defaultSpawnPoint = {
    pos: [227, 239, 53],
    angle: [0, 0, 0],
    worldOrCellDesc: "165a7:Skyrim.esm",
  };

  spawnSystem.spawn = (targetFormId) => {
    const spawnPoint = mp.get(targetFormId, "spawnPoint");
    for (const propName of Object.keys(spawnPoint || defaultSpawnPoint)) {
      mp.set(
        targetFormId,
        propName,
        (spawnPoint || defaultSpawnPoint)[propName]
      );
    }
    actorValues.set(targetFormId, "health", "damage", 0);
    actorValues.set(targetFormId, "magicka", "damage", 0);
    actorValues.set(targetFormId, "stamina", "damage", 0);
    setTimeout(() => {
      mp.set(targetFormId, "isDead", false);
    }, 500);
    utils.log(`${targetFormId.toString(16)} respawns`);
  };

  spawnSystem.updateSpawnPoint = (targetFormId) => {
    mp.set(targetFormId, "spawnPoint", {
      pos: mp.get(targetFormId, "pos"),
      angle: mp.get(targetFormId, "angle"),
      worldOrCellDesc: mp.get(targetFormId, "worldOrCellDesc"),
    });
  };

  utils.hook("onDeath", (pcFormId) => {
    setTimeout(() => {
      spawnSystem.spawn(pcFormId);
    }, 6000);
  });

  utils.hook("onReinit", (pcFormId, options) => {
    if (!mp.get(pcFormId, "spawnPoint") || (options && options.force)) {
      mp.set(pcFormId, "spawnPoint", defaultSpawnPoint);
    }
  });

  return spawnSystem;
};
