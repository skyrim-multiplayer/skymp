import { ObjectReference, Actor, Game, Cell, WorldSpace, TESModPlatform, Debug, printConsole } from 'skyrimPlatform';
import { Movement, RunMode, AnimationVariables, Transform } from './movement';

export let applyMovement = (refr: ObjectReference, m: Movement) => {
    if (teleportIfNeed(refr, m)) return;
    translateTo(refr, m);

    let ac = Actor.from(refr);

    if (ac) {
        ac.setHeadTracking(false);
        ac.stopCombat();

        keepOffsetFromActor(ac, m);

        applySprinting(ac, m.runMode === 'Sprinting');
        applyBlocking(ac, m);
        applySneaking(ac, m.isSneaking);
        applyWeapDrawn(ac, m.isWeapDrawn);
    }
};

let keepOffsetFromActor = (ac: Actor, m: Movement) => {
    let offsetAngle = m.rot[2] - ac.getAngleZ();
    if (Math.abs(offsetAngle) < 5) offsetAngle = 0;

    if (m.runMode === 'Standing') {
        return ac.keepOffsetFromActor(ac, 0, 0, 0, 0, 0, offsetAngle, 1, 1);
    }
    let offset = [
        3 * Math.sin(m.direction / 180 * Math.PI),
        3 * Math.cos(m.direction / 180 * Math.PI), 
        getOffsetZ(m.runMode)
    ];
    
    ac.keepOffsetFromActor(
        ac, offset[0], offset[1], offset[2], 0, 0, offsetAngle, 
        m.runMode === 'Walking' ? 2048 : 1, 1);
};

let getOffsetZ = (runMode: RunMode) => {
    switch (runMode) {
        case 'Walking': return -512;
        case 'Running': return -1024;
    }
    return 0;
};

let applySprinting = (ac: Actor, isSprinting: boolean) => {
    if (ac.isSprinting() != isSprinting) {
        Debug.sendAnimationEvent(ac, isSprinting ? "SprintStart" : "SprintStop");
    }
};

let applyBlocking = (ac: Actor, m: AnimationVariables) => {
    if (ac.getAnimationVariableBool("IsBlocking") != m.isBlocking) {
        Debug.sendAnimationEvent(ac, m.isBlocking ? "BlockStart" : "BlockStop");
        Debug.sendAnimationEvent(ac, m.isSneaking ? "SneakStart" : "SneakStop");
    }
};

let applySneaking = (ac: Actor, isSneaking: boolean) => {
    let currentIsSneaking = ac.isSneaking() || ac.getAnimationVariableBool('IsSneaking');
    if (currentIsSneaking != isSneaking) {
        Debug.sendAnimationEvent(ac, isSneaking ? "SneakStart" : "SneakStop");
    }
};

export let applyWeapDrawn = (ac: Actor, isWeapDrawn: boolean) => {
    if (ac.isWeaponDrawn() !== isWeapDrawn) {
        TESModPlatform.setWeaponDrawnMode(ac, isWeapDrawn ? 1 : 0);
    }
};

let translateTo = (refr: ObjectReference, m: Movement) => {
    let distance = getDistance(getPos(refr), m.pos);
    let time = 0.1;
    if (m.isInJumpState) time = 0.2;
    if (m.runMode !== 'Standing') time = 0.2;
    let speed = distance / time;

    let angleDiff = Math.abs(m.rot[2] - refr.getAngleZ());
    if (m.runMode != 'Standing' || m.isInJumpState || distance > 64 || angleDiff > 80) {
        refr.translateTo(m.pos[0], m.pos[1], m.pos[2], m.rot[0], m.rot[1], m.rot[2], speed, 0);
    }
}

let teleportIfNeed = (refr: ObjectReference, m: Transform) => {
    if (isInDifferentWorldOrCell(refr, m.worldOrCell) || isInDifferentExteriorCell(refr, m.pos)) {
        let cell = Cell.from(Game.getFormEx(m.worldOrCell));
        let world = WorldSpace.from(Game.getFormEx(m.worldOrCell));
        TESModPlatform.moveRefrToPosition(refr, cell, world, m.pos[0], m.pos[1], m.pos[2], m.rot[0], m.rot[1], m.rot[2]);
        return true;
    }
    return false;
}

const cellWidth = 4096;

let isInDifferentExteriorCell = (refr: ObjectReference, pos: number[]) => {
    let currentPos = getPos(refr);
    let playerPos = getPos(Game.getPlayer());
    let targetDistanceToPlayer = getDistance(playerPos, pos);
    let currentDistanceToPlayer = getDistance(playerPos, currentPos);
    return currentDistanceToPlayer > cellWidth && targetDistanceToPlayer <= cellWidth;
}

let isInDifferentWorldOrCell = (refr: ObjectReference, worldOrCell: number) => {
    return worldOrCell !== (refr.getWorldSpace() || refr.getParentCell()).getFormID();
}

let getPos = (refr: ObjectReference)  => {
    return [ refr.getPositionX(), refr.getPositionY(), refr.getPositionZ() ];
}

let getDistance = (a: number[], b: number[]) => {
    let r = 0;
    a.forEach((v, i) => r += Math.pow(a[i] - b[i], 2));
    return Math.sqrt(r);
}