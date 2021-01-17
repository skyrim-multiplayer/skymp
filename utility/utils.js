module.exports = () => {
  const utils = {};

  utils.log = (...args) => {
    console.log.call(console, "[GM]", ...args);
  };
  utils.log("Gamemode init");

  utils.isActor = (formId) => {
    return mp.get(formId, "type") === "MpActor";
  };

  if (!Array.isArray(global.knownEvents)) {
    global.knownEvents = [];
  }
  for (const eventName of global.knownEvents) {
    delete mp[eventName];
  }
  utils.hook = (eventName, callback) => {
    if (!global.knownEvents.includes(eventName)) {
      global.knownEvents.push(eventName);
    }
    const prev = mp[eventName];
    mp[eventName] = (...args) => {
      try {
        const prevRes = prev ? prev(...args) : undefined;
        const callbackRes = callback(...args);
        return callbackRes !== undefined ? callbackRes : prevRes;
      } catch (e) {
        utils.log(`'${eventName}' threw an error: ${e}`);
        if (e["stack"]) {
          utils.log(e["stack"]);
        }
        return undefined;
      }
    };
  };
  return utils;
};
