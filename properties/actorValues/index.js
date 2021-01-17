module.exports = (modules) => {
  const { typecheck, utils, consoleOutput } = modules;

  const actorValues = {};

  const rate = (attr) => {
    return attr === "health" ? "av_healrate" : `av_${attr}rate`;
  };
  const mult = (attr) => {
    return attr === "health" ? "av_healratemult" : `av_${attr}ratemult`;
  };
  const drain = (attr) => {
    return `av_mp_${attr}drain`;
  };

  const updateAttributeCommon = (attr) => `
    const av = "${attr}";
    const ac = ctx.sp.Actor.from(ctx.refr);
    if (!ac) return;

    const base = ctx.value.base || 0;
    const perm = ctx.value.permanent || 0;
    const temp = ctx.value.temporary || 0;
    const targetMax = base + perm + temp;

    const numChangesKey = "${attr}NumChanges";
    const numChanges = ctx.get(numChangesKey);
    if (ctx.state[numChangesKey] !== numChanges) {
      ctx.state[numChangesKey] = numChanges;
      ctx.state.${attr}RegenStart = +Date.now();
    }

    const realTargetDmg = ctx.value.damage || 0;
    let targetDmg = realTargetDmg;

    if (av === "health" || ac.getFormId() == 0x14) {
      const multName = "${mult(attr)}";
      const rateName = "${rate(attr)}";
      const drainName = "${drain(attr)}";

      const additionalRegenMult = 1.0;
      const regenDuration = (+Date.now() - (ctx.state.${attr}RegenStart || 0)) / 1000;
      const healRateMult = ctx.get(multName);
      const healRateMultCurrent = (healRateMult.base || 0)
        + (healRateMult.permanent || 0)
        + (healRateMult.temporary || 0)
        + (healRateMult.damage || 0);
      const healRate = ctx.get(rateName);
      const healRateCurrent = (healRate.base || 0)
        + (healRate.permanent || 0)
        + (healRate.temporary || 0)
        + (healRate.damage || 0);

      const drain = ctx.get(drainName);
      const drainCurrent = (drain.base || 0)
        + (drain.permanent || 0)
        + (drain.temporary || 0)
        + (drain.damage || 0);
      if (drainCurrent) {
        targetDmg += regenDuration * drainCurrent;
      }
      else {
        targetDmg += (regenDuration * additionalRegenMult
          * healRateCurrent * healRateMultCurrent * 0.01 * targetMax * 0.01);
      }

      if (targetDmg > 0) {
        targetDmg = 0;
      }
    }

    const currentPercentage = ac.getActorValuePercentage(av);
    const currentMax = ac.getBaseActorValue(av);

    let targetPercentage = (targetMax + targetDmg) / targetMax;
    if (ctx.get("isDead") && av === "health") {
      targetPercentage = 0;
    }

    const deltaPercentage = targetPercentage - currentPercentage;

    const k = (!targetPercentage || av === "stamina" || av === "magicka") ? 1 : 0.25;

    if (deltaPercentage > 0) {
      ac.restoreActorValue(av, deltaPercentage * currentMax * k);
    }
    else if (deltaPercentage < 0) {
      ac.damageActorValue(av, deltaPercentage * currentMax * k);
    }
  `;

  const updateAttributeNeighbor = (attr) => {
    return attr === "health"
      ? updateAttributeCommon(attr) + `ac.setActorValue("${attr}", 9999);`
      : "";
  };

  const updateAttributeOwner = (attr) => {
    return updateAttributeCommon(attr) + `ac.setActorValue("${attr}", base);`;
  };

  for (const attr of ["health", "magicka", "stamina"]) {
    mp.makeProperty("av_" + attr, {
      isVisibleByOwner: true,
      isVisibleByNeighbors: attr === "health",
      updateNeighbor: updateAttributeNeighbor(attr),
      updateOwner: updateAttributeOwner(attr),
    });
  }

  const avs = [
    "healrate",
    "healratemult",
    "staminarate",
    "staminaratemult",
    "magickarate",
    "magickaratemult",
    "mp_healthdrain",
    "mp_magickadrain",
    "mp_staminadrain",
  ];
  for (const avName of avs) {
    mp.makeProperty("av_" + avName, {
      isVisibleByOwner: true,
      isVisibleByNeighbors: true,
      updateNeighbor: "",
      updateOwner: "",
    });
  }

  const relatedPropNames = [
    "healthNumChanges",
    "magickaNumChanges",
    "staminaNumChanges",
  ];
  for (const propName of relatedPropNames) {
    mp.makeProperty(propName, {
      isVisibleByOwner: true,
      isVisibleByNeighbors: true,
      updateNeighbor: "",
      updateOwner: "",
    });
  }

  const getAvMaximum = (avOps, formId, avName) => {
    let sum = 0;
    for (const modifierName of ["base", "permanent", "temporary"]) {
      sum += avOps.get(formId, avName, modifierName);
    }
    return sum;
  };

  const getAvCurrent = (avOps, formId, avName) => {
    let res = getAvMaximum(avOps, formId, avName);
    res += avOps.get(formId, avName, "damage");
    return res;
  };

  // Basic
  let avOps = {
    set(formId, avName, modifierName, newValue) {
      typecheck.number("newValue", newValue);
      typecheck.avModifier("modifierName", modifierName);
      const propName = "av_" + avName.toLowerCase();
      const value = mp.get(formId, propName);
      value[modifierName] = newValue;
      mp.set(formId, propName, value);
      if (["health", "magicka", "stamina"].includes(avName.toLowerCase())) {
        const propName = `${avName.toLowerCase()}NumChanges`;
        mp.set(formId, propName, 1 + (mp.get(formId, propName) || 0));
      }
    },
    get(formId, avName, modifierName) {
      typecheck.avModifier("modifierName", modifierName);
      const propName = "av_" + avName.toLowerCase();
      const propValue = mp.get(formId, propName);
      if (propValue === undefined) {
        const s = `'${propName}' was '${propValue}' for ${formId.toString(16)}`;
        throw new Error(s);
      }
      return propValue[modifierName] || 0;
    },
  };

  // Damage limit
  avOps = {
    parent: avOps,
    set(formId, avName, modifierName, newValue) {
      if (modifierName == "damage") {
        if (newValue > 0) {
          newValue = 0;
        } else if (newValue < -getAvMaximum(this.parent, formId, avName)) {
          newValue = -getAvMaximum(this.parent, formId, avName);
        }
      }
      this.parent.set(formId, avName, modifierName, newValue);
    },
    get(...args) {
      return this.parent.get(...args);
    },
  };

  const timeSource = {
    getSecondsPassed() {
      if (!this.startMoment) {
        this.startMoment = Date.now();
      }
      return (+Date.now() - +this.startMoment) / 1000.0;
    },
  };

  // Regen
  let regen = (avOps, avNameTarget, avNameRate, avNameRateMult, avNameDrain) =>
    new Object({
      parent: avOps,
      set(formId, avName, modifierName, newValue) {
        let dangerousAvNames = [
          avNameTarget,
          avNameRate,
          avNameRateMult,
          avNameDrain,
        ];
        dangerousAvNames = dangerousAvNames.map((x) => x.toLowerCase());
        if (dangerousAvNames.includes(avName.toLowerCase())) {
          this.applyRegenerationToParent(formId);
        }
        this.parent.set(formId, avName, modifierName, newValue);
      },
      get(formId, avName, modifierName) {
        const drain = getAvCurrent(this.parent, formId, avNameDrain);
        const realValue = this.parent.get(formId, avName, modifierName);
        if (avName.toLowerCase() === avNameTarget.toLowerCase()) {
          if (modifierName === "damage") {
            const avMax = getAvMaximum(this.parent, formId, avName);
            const regenDuration =
              timeSource.getSecondsPassed() - this.getSecondsMatched(formId);
            const rate = getAvCurrent(this.parent, formId, avNameRate);
            const rateMult = getAvCurrent(this.parent, formId, avNameRateMult);
            let damageMod = realValue;
            if (drain) {
              damageMod += regenDuration * drain;
            } else {
              damageMod +=
                regenDuration * rate * rateMult * 0.01 * avMax * 0.01;
            }
            return Math.min(0, damageMod);
          }
        }
        return realValue;
      },
      applyRegenerationToParent(formId) {
        const damageAfterRegen = this.get(formId, avNameTarget, "damage");
        this.parent.set(formId, avNameTarget, "damage", damageAfterRegen);
        this.setSecondsMatched(formId, timeSource.getSecondsPassed());
      },
      getSecondsMatched(formId) {
        return (this.secondsMatched && this.secondsMatched[formId]) || 0;
      },
      setSecondsMatched(formId, secondsMatched) {
        if (!this.secondsMatched) {
          this.secondsMatched = {};
        }
        this.secondsMatched[formId] = secondsMatched;
      },
    });
  avOps = regen(avOps, "health", "healrate", "healratemult", "mp_healthdrain");
  avOps = regen(
    avOps,
    "magicka",
    "magickarate",
    "magickaratemult",
    "mp_magickadrain"
  );
  avOps = regen(
    avOps,
    "stamina",
    "staminarate",
    "staminaratemult",
    "mp_staminadrain"
  );

  // Scaling
  avOps = {
    parent: avOps,
    set(formId, avName, modifierName, newValue) {
      let oldMaximum, newMaximum;

      oldMaximum = getAvMaximum(this.parent, formId, avName);
      this.parent.set(formId, avName, modifierName, newValue);
      newMaximum = getAvMaximum(this.parent, formId, avName);

      const k = newMaximum / oldMaximum;
      if (isFinite(k) && k != 1) {
        this.multiplyDamage(formId, avName, k);
      }
    },
    get(...args) {
      return this.parent.get(...args);
    },
    multiplyDamage(formId, avName, k) {
      const previousDamage = this.parent.get(formId, avName, "damage");
      this.parent.set(formId, avName, "damage", previousDamage * k);
    },
  };

  actorValues.set = (...args) => avOps.set(...args);
  actorValues.get = (...args) => avOps.get(...args);
  actorValues.getMaximum = (...args) => getAvMaximum(avOps, ...args);
  actorValues.getCurrent = (...args) => getAvCurrent(avOps, ...args);

  actorValues.flushRegen = (formId, avName) => {
    const damageModAfterRegen = avOps.get(formId, avName, "damage");
    avOps.set(formId, avName, "damage", damageModAfterRegen);
  };

  actorValues.setDefaults = (formId, options) => {
    const force = options && options.force;
    if (utils.isActor(formId)) {
      if (mp.get(formId, "isDead") === undefined || force) {
        mp.set(formId, "isDead", false);
      }
      for (const avName of ["health", "magicka", "stamina"]) {
        if (!mp.get(formId, "av_" + avName) || force) {
          mp.set(formId, "av_" + avName, { base: 100 });
        }
      }
      for (const avName of ["healrate", "magickarate", "staminarate"]) {
        if (!mp.get(formId, "av_" + avName) || force) {
          mp.set(formId, "av_" + avName, { base: 5 });
        }
      }
      for (const avName of [
        "healratemult",
        "magickaratemult",
        "staminaratemult",
      ]) {
        if (!mp.get(formId, "av_" + avName) || force) {
          mp.set(formId, "av_" + avName, { base: 100 });
        }
      }
      for (const avName of [
        "mp_healthdrain",
        "mp_magickadrain",
        "mp_staminadrain",
      ]) {
        if (!mp.get(formId, "av_" + avName) || force) {
          mp.set(formId, "av_" + avName, { base: 0 });
        }
      }
    }
  };

  utils.hook("onReinit", (formId, options) => {
    actorValues.setDefaults(formId, options);

    /*if (utils.isActor(formId)) {
      const wouldDie = actorValues.getCurrent(formId, "Health") <= 0;
      if (wouldDie && !mp.get(formId, "isDead")) {
        mp.onDeath(formId);
      }
    }*/
  });

  utils.hook("_onHit", (pcFormId, eventData) => {
    let damageMod = -25;
    const avName = "health";

    const damage = actorValues.get(eventData.target, avName, "damage");

    const agressorDead =
      actorValues.getCurrent(eventData.agressor, "Health") <= 0;
    if (damageMod < 0 && agressorDead) {
      utils.log("Dead characters can't hit");
      return;
    }

    const greenZone = "165a7:Skyrim.esm";
    if (mp.get(eventData.agressor, "worldOrCellDesc") === greenZone) {
      const msgs = [
        "Вы с удивлением замечаете, что оставили лишь царапину",
        "Вы не верите своим глазам. Боги отвели удар от цели",
        "Вы чувствуете, что Кинарет наблюдает за вашими действиями",
      ];
      const i = Math.floor(Math.random() * msgs.length);
      consoleOutput.printNote(pcFormId, msgs[i]);
      damageMod = i === 0 ? -1 : 0;
    }

    const newDamageModValue = damage + damageMod;
    actorValues.set(eventData.target, avName, "damage", newDamageModValue);

    const wouldDie =
      actorValues.getMaximum(eventData.target, "Health") + newDamageModValue <=
      0;
    if (wouldDie && !mp.get(eventData.target, "isDead")) {
      mp.onDeath(eventData.target);
    }
  });

  mp.makeEventSource(
    "_onLocalDeath",
    `
    ctx.sp.on("update", () => {
      const isDead = ctx.sp.Game.getPlayer().getActorValuePercentage("health") === 0;
      if (ctx.state.wasDead !== isDead) {
        if (isDead) {
          ctx.sendEvent();
        }
        ctx.state.wasDead = isDead;
      }
    });
  `
  );

  utils.hook("_onLocalDeath", (pcFormId) => {
    utils.log("_onLocalDeath", pcFormId.toString(16));
    const max = actorValues.getMaximum(pcFormId, "Health");
    actorValues.set(pcFormId, "Health", "damage", -max);
    mp.onDeath(pcFormId);
  });

  const sprintAttr = "stamina";
  utils.hook("_onSprintStateChange", (pcFormId, newState) => {
    switch (newState) {
      case "start":
        actorValues.set(pcFormId, `mp_${sprintAttr}drain`, "base", -10);
        const damageMod = actorValues.get(pcFormId, sprintAttr, "damage");
        actorValues.set(pcFormId, sprintAttr, "damage", damageMod - 10);
        break;
      case "stop":
        actorValues.set(pcFormId, `mp_${sprintAttr}drain`, "base", 0);
        break;
      default:
        break;
    }
  });
  utils.hook("_onPowerAttack", (pcFormId) => {
    const damage = actorValues.get(pcFormId, sprintAttr, "damage");
    const damageMod = -35;
    actorValues.set(pcFormId, sprintAttr, "damage", damage + damageMod);
  });
  utils.hook("_onBash", (pcFormId) => {
    const damage = actorValues.get(pcFormId, sprintAttr, "damage");
    const damageMod = -35;
    actorValues.set(pcFormId, sprintAttr, "damage", damage + damageMod);
  });

  return actorValues;
};
