const assert = require("node:assert");

const main = async () => {
  var actorId = mp.createActor(0, [0, 0, 0], 0, 0x0000003c);
  var actor = { type: "form", desc: mp.getDescFromId(actorId) };
  var gold001 = { type: "espm", desc: mp.getDescFromId(0x0000000f) };

  var events = [];

  mp["onPapyrusEvent:OnItemAdded"] = function () {
    events.push(arguments);
  };

  mp.callPapyrusFunction("method", "ObjectReference", "AddItem", actor, [gold001, 100, false, null]);

  var inv = mp.get(actorId, "inventory").entries;
  var goldEntries = inv.filter((entry) => entry.baseId === 0x0000000f);
  assert.strictEqual(goldEntries.length, 1);
  assert.strictEqual(goldEntries[0].count, 100);

  assert.strictEqual(events.length, 1);
  assert.strictEqual(events[0][0], actorId);
  assert.deepEqual(events[0][1], { type: "espm", desc: mp.getDescFromId(0x0000000f) });
  assert.strictEqual(events[0][2], 100);
  assert.strictEqual(events[0][3], null);
  assert.strictEqual(events[0][4], null);
};

main().then(() => {
  console.log("Test passed!");
  process.exit(0);
}).catch((err) => {
  console.log("Test failed!")
  console.error(err);
  process.exit(1);
});
