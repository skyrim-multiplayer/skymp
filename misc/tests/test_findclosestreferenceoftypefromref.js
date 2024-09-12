const assert = require("node:assert");

const main = async () => {
  const akFormToPlace = { type: 'espm', desc: mp.getDescFromId(0x7) };
  const barrelInWhiterun = 0x4cc2d;
  const barrelObject = { type: 'form', desc: mp.getDescFromId(barrelInWhiterun) };

  const barrelBaseForm = { type: 'espm', desc: mp.get(barrelInWhiterun, 'baseDesc') };

  let searchRes;

  // expect no actor to be found around the barrel
  searchRes = mp.callPapyrusFunction("global", "Game", "FindClosestReferenceOfTypeFromRef", null, [akFormToPlace, barrelObject, 100]);
  assert.deepEqual(searchRes, null);

  // placing
  const placedActor = mp.callPapyrusFunction("method", "ObjectReference", "PlaceAtMe", barrelObject, [akFormToPlace, 1, true, false]);

  // expect the actor to be found around the barrel
  searchRes = mp.callPapyrusFunction("global", "Game", "FindClosestReferenceOfTypeFromRef", null, [akFormToPlace, barrelObject, 100]);
  assert.deepEqual(searchRes, placedActor);

  // vice-versa
  searchRes = mp.callPapyrusFunction("global", "Game", "FindClosestReferenceOfTypeFromRef", null, [barrelBaseForm, placedActor, 100]);
  assert.deepEqual(searchRes, barrelObject);
};

main().then(() => {
  console.log("Test passed!");
  process.exit(0);
}).catch((err) => {
  console.log("Test failed!")
  console.error(err);
  process.exit(1);
});
