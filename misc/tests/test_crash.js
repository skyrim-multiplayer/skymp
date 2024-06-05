const assert = require("node:assert");
const fs = require("node:fs");

const main = async () => {
  // instead of [0,0,0] there should be only angleZ
  // in older versions of the server, this would crash the server

  let thrown = false;
  let unexpectedError = "";
  try {
    mp.createActor(0, [1, 1, 1], [0, 0, 0], "3c:Skyrim.esm");
  } catch (e) {
    if (e.message.includes("Expected 'angleZ' to be number, but got '[0,0,0]'")) {
      thrown = true;
    }
    else {
      unexpectedError = e.message;
    }
  }

  assert.equal(unexpectedError, "");
  assert.equal(thrown, true);
};

main().then(() => {
  console.log("Test passed!");
  process.exit(0);
}).catch((err) => {
  console.log("Test failed!")
  console.error(err);
  process.exit(1);
});
