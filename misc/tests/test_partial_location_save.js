const assert = require("node:assert");
const fs = require("node:fs");

const main = async () => {
  const crabActorId = 0xDC558;
 
  // trigger lazy load of the actor
  mp.get(crabActorId, "isDead");

  // Imagine the actor died and was teleported to a different cell
  // Then the server restarts

  assert.strictEqual(mp.get(crabActorId, "isDead"), true);

  const initialPos = [
    25.0,
    -707.0,
    0.0
  ];
  const initialCellOrWorldDesc = "37ee0:Skyrim.esm";

  assert.deepEqual(mp.get(crabActorId, "pos"), initialPos);
  assert.strictEqual(mp.get(crabActorId, "worldOrCellDesc"), initialCellOrWorldDesc);


  let respawned = false;

  mp.onRespawn = (actorId) => {
    if (actorId === crabActorId) {
      respawned = true;
    }
  };

  // spawnDelay should be 0 so just wait a bit
  await new Promise((resolve) => setTimeout(resolve, 1000));

  assert.strictEqual(respawned, true);


  assert.strictEqual(mp.get(crabActorId, "isDead"), false);

  const expectedPos = [
    9409.498046875,
    9289.421875,
    -5150.0
  ];

  assert.deepEqual(mp.get(crabActorId, "pos"), expectedPos);

  // mp.set(crabActorId, "private.foo", "bar"); // force save

  const expectedCellOrWorldDesc = "3c:Skyrim.esm";

  const cellOrWorldDesc = mp.get(crabActorId, "worldOrCellDesc");
  assert.strictEqual(cellOrWorldDesc, expectedCellOrWorldDesc);

  // database flush
  await new Promise((resolve) => setTimeout(resolve, 5000));

  assert.deepEqual(mp.get(crabActorId, "pos"), expectedPos);

  const changeForm = JSON.parse(fs.readFileSync("world/changeForms/dc558_Skyrim.esm.json", "utf8"));

  assert.strictEqual(changeForm.worldOrCellDesc, expectedCellOrWorldDesc);
  assert.deepEqual(changeForm.position, expectedPos);
};

main().then(() => {
  console.log("Test passed!");
  process.exit(0);
}).catch((err) => {
  console.log("Test failed!")
  console.error(err);
  process.exit(1);
});
