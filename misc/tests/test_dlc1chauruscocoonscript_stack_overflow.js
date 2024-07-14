const assert = require("node:assert");

const main = async () => {
  // dlc1chauruscocoonscript infinite recursion next tick after calling getFormEx
  mp.callPapyrusFunction("global", "Game", "getFormEx", null, [33565538]);

  await new Promise((resolve) => setTimeout(resolve, 500));
};

main().then(() => {
  console.log("Test passed!");
  process.exit(0);
}).catch((err) => {
  console.log("Test failed!")
  console.error(err);
  process.exit(1);
});

// Will output:
// [error] ActivePexInstance::StartFunction - Stack overflow in script DLC1ChaurusCocoonScript, returning None
// This is a sign that server isn't gonna crash

// Triggers only after 2-nd start of the server (needs world to be initially here)
