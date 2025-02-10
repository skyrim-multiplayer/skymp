const assert = require("node:assert");

const main = async () => {
    var actorId1 = mp.createActor(0, [0, 0, 0], 0, 0x0000003c);
    var actorId2 = mp.createActor(0, [0, 0, 0], 0, 0x0000003c);

    mp.set(actorId1, "inventory", { entries: [{ baseId: 0xf, count: 1000 }, { baseId: 0x12eb7, count: 20 }] });
    mp.set(actorId2, "inventory", { entries: [] });

    mp.callPapyrusFunction("method", "ObjectReference", "RemoveAllItems",
        { type: "form", desc: mp.getDescFromId(actorId1) },
        [{ type: "form", desc: mp.getDescFromId(actorId2) }, false, false]);

    assert.deepEqual(mp.get(actorId1, "inventory"), { entries: [] });
    assert.deepEqual(mp.get(actorId2, "inventory"), { entries: [{ baseId: 0xf, count: 1000 }, { baseId: 0x12eb7, count: 20 }] });
};

main().then(() => {
    console.log("Test passed!");
    process.exit(0);
}).catch((err) => {
    console.log("Test failed!")
    console.error(err);
    process.exit(1);
});
