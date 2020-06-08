import { findConsoleCommand, printConsole, once, Spell, HttpClient, on, Utility, Form, Actor, ObjectReference, Weapon, Debug } from "skyrimPlatform";

import { Game } from "skyrimPlatform";

import * as ch from "chai";
declare var chai: typeof ch;
var global: any = {};
let expect: typeof ch.expect;

let runTest = (testFn: () => void) => {
    let cloadflare = new HttpClient('cdnjs.cloudflare.com', 80);
    let p = cloadflare.get('/ajax/libs/chai/4.2.0/chai.js');
    p.then(res => {
        eval(res.body);
        expect = global.chai.expect;
        once('update', testFn);
    });
};

let testCallStaticNoArgs = () => {
    // Reset values before testing
    Game.setPerkPoints(0);
            
    expect(Game.getPlayer()).not.to.eql(null);
    expect(Game.getPlayer().getFormID()).to.eql(0x14);
        
    expect(Game.getFormEx(0x14)).not.to.eql(null);
    expect(Game.getForm(0x14)).not.to.eql(null);
            
    expect(Game.getForm(0x14).getFormID()).to.eql(Game.getPlayer().getFormID());
    expect(Game.getFormEx(0x14).getFormID()).to.eql(Game.getPlayer().getFormID());
    expect(Game.getFormEx(-0x14)).to.eql(null);
        
    expect(Game.getForm(0x7)).not.to.eql(null);
    expect(Game.getPlayer().getBaseObject().getFormID()).to.eql(Game.getForm(0x7).getFormID());

    Game.enableFastTravel(false);
    expect(Game.isFastTravelEnabled()).to.be.false;
    Game.enableFastTravel(true);
    expect(Game.isFastTravelEnabled()).to.be.true;

    expect(Game.getPerkPoints()).to.eql(0);
    Game.setPerkPoints(1);
    expect(Game.getPerkPoints()).to.eql(1);
    Game.addPerkPoints(2);
    expect(Game.getPerkPoints()).to.eql(3);
    Game.modPerkPoints(-3);
    expect(Game.getPerkPoints()).to.eql(0);
            
    let x = Game.getPlayer().getPositionX();
    let y = Game.getPlayer().getPositionY();
    let z = Game.getPlayer().getPositionZ();
    expect(Game.findClosestActor(x, y, z, 1000).getFormID()).to.be.equal(Game.getPlayer().getFormID());
    expect(Game.findClosestActor(x, y, z + 200, 10)).to.be.null;

    expect(Game.from(null)).to.be.null;
    expect(Game.from(Game.getPlayer())).to.be.null;
    expect(Form.from(null)).to.be.null;
    expect(Actor.from(Game.getPlayer())).not.to.be.null;
    expect(ObjectReference.from(Game.getPlayer())).not.to.be.null;
    expect(Form.from(Game.getPlayer())).not.to.be.null;
    expect(Actor.from(Game.getForm(0x14))).not.to.be.null;
    expect(Actor.from(Game.getForm(0x7))).to.be.null;
    expect(Form.from(Game.getForm(0x7))).not.to.be.null;
    expect(Weapon.from(Game.getForm(0x12eb7))).not.to.be.null;
    expect(Spell.from(Game.getForm(0x6f953))).not.to.be.null;
    expect(Weapon.from(Game.getForm(0x6f953))).to.be.null;
    expect(Form.from(Game.getForm(0x6f953))).not.to.be.null;
    expect(Form.from(Spell.from(Game.getForm(0x6f953)))).not.to.be.null;
    expect(ObjectReference.from(Game.getForm(0x14))).not.to.be.null;

    Game.getPlayer().getBaseObject().setName('Сергей Бубович');
    expect(Game.getPlayer().getBaseObject().getName()).to.be.eql('Сергей Бубович');
    Game.getPlayer().getBaseObject().setName('Pepe');
    expect(Game.getPlayer().getBaseObject().getName()).to.be.eql('Pepe');
    
    Debug.sendAnimationEvent(Game.getPlayer(), "jumpstandingstart");

    let ffRefr = Game.getPlayer().placeAtMe(Game.getPlayer().getBaseObject(), 1, false, true);
    let ffRefrId = ffRefr.getFormID();
    expect(ffRefrId).to.be.greaterThan(0xff000000);
    expect(Game.getFormEx(ffRefrId)).not.to.be.null;
    expect(Game.getFormEx(ffRefrId).getFormID()).to.be.eql(ffRefrId);

    let ironSword = Game.getFormEx(0x12eb7);
    let n = Game.getPlayer().getItemCount(ironSword);
    let addItemReturnValue = Game.getPlayer().addItem(ironSword, 1, true);
    expect(Game.getPlayer().getItemCount(ironSword)).to.be.eql(n + 1);
    expect(addItemReturnValue).to.be.null;

    let barrel = Game.getFormEx(0x60752);
    let barrelRef = Game.getPlayer().placeAtMe(barrel, 1, true,false);
    expect(barrelRef).to.not.be.null;

    expect(barrelRef.getItemCount(ironSword)).to.be.eql(0);
    barrelRef.addItem(ironSword, 10, false);
    expect(barrelRef.getItemCount(ironSword)).to.be.eql(10);
    barrelRef.removeItem(ironSword, 5, true, Game.getPlayer());
    expect(barrelRef.getItemCount(ironSword)).to.be.eql(5);
    expect(Game.getPlayer().getItemCount(ironSword)).to.eql(5 + n + 1);

    Game.getPlayer().addItem(barrelRef, 10, false);
    Game.getPlayer().removeItem(barrelRef, 10, false, null);

    let blackreachWeather = Game.getFormEx(0x48c14);
    Game.getPlayer().addItem(blackreachWeather, 1, false);
    Game.getPlayer().removeItem(blackreachWeather, 1, false, null);

    expect(barrelRef.getItemCount(ironSword)).to.not.eql(0);
    barrelRef.removeItem(ironSword, -1, true, null);
    expect(barrelRef.getItemCount(ironSword)).to.be.eql(0);

    printConsole('Test passed');
};

export let main = () => {
    let cmd = findConsoleCommand(' ConfigureUM');
    cmd.shortName = cmd.longName = 'test';
    cmd.execute = (selectedRefId: number, testName: string) => {
        if (testName === 'all') {
            once('update', () => runTest(testCallStaticNoArgs));
        }
        return false;
    };
};
