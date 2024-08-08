const assert = require("node:assert");

const main = async () => {
  var activationCasterId = mp.createActor(0, [0, 0, 0], 0, 0x3c);
  var activationTargetId = mp.createActor(0, [0, 0, 0], 0, 0x3c);

  var activationCaster = { type: 'form', desc: mp.getDescFromId(activationCasterId) };
  var activationTarget = { type: 'form', desc: mp.getDescFromId(activationTargetId) };

  var abDefaultProcessingOnly = false;

  var activations = [];

  mp.onActivate = (target, caster) => {
    activations.push({ target, caster });
  };

  mp.callPapyrusFunction("method", "ObjectReference", "Activate", activationTarget, [
    activationCaster,
    abDefaultProcessingOnly
  ]);

  assert.strictEqual(activations.length, 1);
  assert.deepEqual(activations[0].caster, activationCasterId);
  assert.deepEqual(activations[0].target, activationTargetId);
};

main().then(() => {
  console.log("Test passed!");
  process.exit(0);
}).catch((err) => {
  console.log("Test failed!")
  console.error(err);
  process.exit(1);
});
