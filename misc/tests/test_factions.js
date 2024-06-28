const assert = require("node:assert");

const main = async () => {
  if (mp.get(0x15A6E, "type") === "MpActor") {
    let selfObject = { type: 'form', desc: mp.getDescFromId(0x15A6E) }
    let falmerFaction = mp.callPapyrusFunction("global", "Game", "GetFormEx", null, [0x3E096])

    let fact = mp.callPapyrusFunction("method", "Actor", "GetFactions", selfObject, [-128, 127])
    assert.strictEqual(fact.length, 2)
    assert.strictEqual(fact[0].desc, "13:Skyrim.esm")
    assert.strictEqual(fact[0].type, "espm")
    assert.strictEqual(fact[1].desc, "3e096:Skyrim.esm")
    assert.strictEqual(fact[1].type, "espm")

    let isInFalmerFaction = mp.callPapyrusFunction("method", "Actor", "IsInFaction", selfObject, [falmerFaction])
    assert.strictEqual(isInFalmerFaction, true)

    mp.callPapyrusFunction("method", "Actor", "RemoveFromFaction", selfObject, [falmerFaction])

    fact = mp.callPapyrusFunction("method", "Actor", "GetFactions", selfObject, [-128, 127])
    assert.strictEqual(fact.length, 1)
    assert.strictEqual(fact[0].desc, "13:Skyrim.esm")
    assert.strictEqual(fact[0].type, "espm")

    mp.callPapyrusFunction("method", "Actor", "AddToFaction", selfObject, [falmerFaction])

    fact = mp.callPapyrusFunction("method", "Actor", "GetFactions", selfObject, [-128, 127])
    assert.strictEqual(fact.length, 2)
    assert.strictEqual(fact[0].desc, "13:Skyrim.esm")
    assert.strictEqual(fact[0].type, "espm")
    assert.strictEqual(fact[1].desc, "3e096:Skyrim.esm")
    assert.strictEqual(fact[1].type, "espm")
  }
  else {
      assert.fail("0x15A6E is not MpActor")
  }
};

main().then(() => {
  console.log("Test passed!");
  process.exit(0);
}).catch((err) => {
  console.log("Test failed!")
  console.error(err);
  process.exit(1);
});
