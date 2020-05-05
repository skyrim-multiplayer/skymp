import { on, printConsole, Game, Debug, Actor, Form, ActorBase, ObjectReference, Weapon, Spell } from "../skyrimPlatform"


function assertEqualForms(x: Form, y: Form) {
    [x, y].forEach(v => {
        if (!v && v !== null) {
            throw new Error(`Form must be an object or null, but it's ${typeof v}`);
        }
    });
    let xId = x ? x.getFormID() : -1;
    let yId = y ? y.getFormID() : -1;
    let unequal = xId !== yId;
    if (unequal) {
        throw new Error(`Expected ${x} to be equal ${y}`);
    }
}

function assertEqual(x: any, y: any) {
    let unequal = x !== y;
    if (unequal) {
        throw new Error(`Expected ${x} to be equal ${y}`);
    }
}

let runTest = (f: any) => {
    try {
        f();
    }
    catch(e) {
        throw new Error(`Test '${f.name}' failed:\n${e.stack}`);
    }
}

function testFindClosestActor() {
    // Reset values before testing
    Game.setPerkPoints(0);

    assertEqual(!!Game.getPlayer(), true);
    assertEqual(Game.getPlayer().getFormID(), 0x14);

    assertEqual(!!Game.getFormEx(0x14), true);
    assertEqual(!!Game.getForm(0x14), true);
    assertEqualForms(Game.getForm(0x14), Game.getPlayer());
    assertEqualForms(Game.getFormEx(0x14), Game.getPlayer());

    assertEqualForms(Game.getFormEx(-0x14), null);

    assertEqual(!!Game.getForm(0x7), true);
    assertEqualForms(Game.getPlayer().getBaseObject(), Game.getForm(0x7));

    Game.enableFastTravel(false);
    assertEqual(Game.isFastTravelEnabled(), false);
    Game.enableFastTravel(true);
    assertEqual(Game.isFastTravelEnabled(), true);

    assertEqual(Game.getPerkPoints(), 0);
    Game.setPerkPoints(1);
    assertEqual(Game.getPerkPoints(), 1);
    Game.addPerkPoints(2);
    assertEqual(Game.getPerkPoints(), 3);
    Game.modPerkPoints(-3);
    assertEqual(Game.getPerkPoints(), 0);

    let x = Game.getPlayer().getPositionX();
    let y = Game.getPlayer().getPositionY();
    let z = Game.getPlayer().getPositionZ();
    assertEqual(Game.findClosestActor(x, y, z, 1000), Game.getPlayer());
    assertEqual(Game.findClosestActor(x, y, z + 200, 10), null);

    assertEqual(Game.from(null), null);
    assertEqual(Game.from(Game.getPlayer()), null);
    assertEqual(Form.from(null), null);
    //assertEqual(Actor.from(Game.getPlayer()), true);
    //assertEqual(ObjectReference.from(Game.getPlayer()), true);
    //assertEqual(Form.from(Game.getPlayer()), true);
    assertEqual(!!Actor.from(Game.getForm(0x14)), true);
    assertEqual(!!Actor.from(Game.getForm(0x7)), false);
    assertEqual(!!Form.from(Game.getForm(0x7)), true);
    assertEqual(!!Weapon.from(Game.getForm(0x12eb7)), true);
    assertEqual(!!Spell.from(Game.getForm(0x6f953)), true);
    assertEqual(!!Weapon.from(Game.getForm(0x6f953)), false);
    assertEqual(!!Form.from(Game.getForm(0x6f953)), true);
    //assertEqual(!!Form.from(Spell.from(Game.getForm(0x6f953))), true);

    //Game.getPlayer().setPosition(0,0,0);
};

let runTests = () => {
    for (let i = 0; i < 20; ++i) {
        printConsole();
    }
    runTest(testFindClosestActor);
    printConsole('Tests passed');
};

let isFirstUpdate = true;

export let main = () => {
    on('update', () => {
        if (!isFirstUpdate) return;
        isFirstUpdate = false;
        runTests();
    });
};