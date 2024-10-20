// This test was not checked
const assert = require("node:assert");

const main = async () => {
  const barrelInWhiterunId = 0x0004cc2d;
  const baseNpcId = 0x00023abe;

  const barrelInWhiterun = mp.callPapyrusFunction("global", "Game", "getFormEx", null, [barrelInWhiterunId]);
  const baseNpc = mp.callPapyrusFunction("global", "Game", "getFormEx", null, [baseNpcId]);
  const placedNpc = mp.callPapyrusFunction("method", "ObjectReference", "placeAtMe", barrelInWhiterun, [baseNpc, 1, false, false]);

  const placedNpcSpawnPoint = mp.get(mp.getIdFromDesc(placedNpc), "spawnPoint");
  const barrelInWhiterunLocationalData = mp.get(mp.getIdFromDesc(barrelInWhiterun), "locationalData");

  assert.deepEqual(placedNpcSpawnPoint, barrelInWhiterunLocationalData);
};

main().then(() => {
  console.log("Test passed!");
  process.exit(0);
}).catch((err) => {
  console.log("Test failed!")
  console.error(err);
  process.exit(1);
});
