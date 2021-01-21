import { MP } from '../types';
import { utils } from '../utility';

declare const mp: MP;

const updateNeighbor = `
const ac = ctx.sp.Actor.from(ctx.refr);
const isDead = ctx.value;
if (isDead) {
  ac.endDeferredKill();
  ac.kill(null);
}
else {
  ac.startDeferredKill();
}

if (!isDead && ac.isDead()) {
  ctx.respawn();
}
`;

const updateOwner = `
const ac = ctx.sp.Actor.from(ctx.refr);
ac.startDeferredKill();

const value = ctx.value;
if (value !== ctx.state.value) {
  const die = !!value;
  if (die) {
    const pos = [
      ac.getPositionX(), ac.getPositionY(), ac.getPositionZ()
    ];

    // Everyone should stop combat with us
    for (let i = 0; i < 200; ++i) {
      const randomActor = ctx.sp.Game.findRandomActor(pos[0], pos[1], pos[2], 10000);
      if (!randomActor) continue;
      const tgt = randomActor.getCombatTarget();
      if (!tgt || tgt.getFormID() !== 0x14) continue;
      randomActor.stopCombat();
    }

    ac.pushActorAway(ac, 0);
  }

  if (!die) {
    ctx.sp.Debug.sendAnimationEvent(ac, "GetUpBegin");
  }

  ctx.state.value = value;
}
`;

export const initIsDead = () => {
	mp.makeProperty('isDead', {
		isVisibleByOwner: true,
		isVisibleByNeighbors: true,
		updateNeighbor: updateNeighbor,
		updateOwner: updateOwner,
	});

	utils.hook('onDeath', (pcFormId: number) => {
		mp.set(pcFormId, 'isDead', true);
		utils.log(`${pcFormId.toString(16)} died`);
	});
};
