const assert = require("node:assert");

const main = async () => {
  const actorId = mp.createActor(0, [1, 1, 1], 0, 0x3c);
  const actorObject = { type: 'form', desc: mp.getDescFromId(actorId) };

  const timeWas = Date.now();

  mp.callPapyrusFunction("method", "Form", "RegisterForSingleUpdate", actorObject, [0]);
  mp.callPapyrusFunction("method", "Form", "RegisterForSingleUpdate", actorObject, [0.1]);
  mp.callPapyrusFunction("method", "Form", "RegisterForSingleUpdate", actorObject, [0.2]);
  mp.callPapyrusFunction("method", "Form", "RegisterForSingleUpdate", actorObject, [0.5]);

  let receivedEvents = [];

  mp["onPapyrusEvent:OnUpdate"] = () => {
    receivedEvents.push(Date.now() - timeWas);
  };

  await new Promise((resolve) => setTimeout(resolve, 1000));

  // Each call to RegisterForSingleUpdate overrides the previous one for a given form
  assert.strictEqual(receivedEvents.length, 1);
  assert(receivedEvents[0] >= 500 && receivedEvents[0] <= 1000);
};

main().then(() => {
  console.log("Test passed!");
  process.exit(0);
}).catch((err) => {
  console.log("Test failed!")
  console.error(err);
  process.exit(1);
});
