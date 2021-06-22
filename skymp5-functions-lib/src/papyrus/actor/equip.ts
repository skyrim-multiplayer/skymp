import { evalClient } from '../../properties/eval';
import { Ctx } from '../../types/ctx';
import { Mp, PapyrusObject, PapyrusValue } from '../../types/mp';
import { FunctionInfo } from '../../utils/functionInfo';
import { getObject, getBoolean, getNumber } from '../../utils/papyrusArgs';

export const equipItem = (mp: Mp, self: PapyrusObject, args: PapyrusValue[]) => {
  const selfId = mp.getIdFromDesc(self.desc);
  const item = getObject(args, 0);
  const itemId = mp.getIdFromDesc(item.desc);
  const preventRemoval = args[1] ? getBoolean(args, 1) : false;
  const silent = args[2] ? getBoolean(args, 2) : false;

  const countExist = mp.callPapyrusFunction('method', 'Actor', 'GetItemCount', self, [item]);

  if (countExist === 0) {
    // TODO: don`t work with default args
    // * mp.callPapyrusFunction('method', 'Actor', 'AddItem', self, [item]) don`t work
    mp.callPapyrusFunction('method', 'Actor', 'AddItem', self, [item, 1, false]);
  }

  const func = (ctx: Ctx, itemId: number, preventRemoval: boolean, silent: boolean) => {
    ctx.sp.once('update', () => {
      const form = ctx.sp.Game.getFormEx(itemId);
      ctx.sp.Game.getPlayer()?.equipItem(form, preventRemoval, silent);
    });
  };
  evalClient(mp, selfId, new FunctionInfo(func).getText({ itemId, preventRemoval, silent }));

  if (!silent) {
    //notification
  }
};

export const equipItemEx = (mp: Mp, self: PapyrusObject, args: PapyrusValue[]) => {
  const selfId = mp.getIdFromDesc(self.desc);
  const item = getObject(args, 0);
  const itemId = mp.getIdFromDesc(item.desc);
  const slot = args[1] ? getNumber(args, 1) : 0;
  const preventUnequip = args[2] ? getBoolean(args, 2) : false;
  const equipSound = args[3] ? getBoolean(args, 3) : true;

  const func = (ctx: Ctx, itemId: number, slot: number, preventUnequip: boolean, equipSound: boolean) => {
    ctx.sp.once('update', () => {
      const form = ctx.sp.Game.getFormEx(itemId);
      ctx.sp.Game.getPlayer()?.equipItemEx(form, slot, preventUnequip, equipSound);
    });
  };
  evalClient(mp, selfId, new FunctionInfo(func).getText({ itemId, slot, preventUnequip, equipSound }));

  if (!equipSound) {
    // sound
  }
};

export const equipItemById = (mp: Mp, self: PapyrusObject, args: PapyrusValue[]) => {};

// TODO: not correct work boolean return
export const isEquipped = (mp: Mp, self: PapyrusObject, args: PapyrusValue[]) => {
  const selfId = mp.getIdFromDesc(self.desc);
  const item = getObject(args, 0);
  const itemId = mp.getIdFromDesc(item.desc);

  const eq = mp.get(selfId, 'equipment');
  if (eq.inv.entries.findIndex((item) => item.baseId === itemId && item.worn) >= 0) {
    return true;
  }
  return;
};

export const unequipItem = (mp: Mp, self: PapyrusObject, args: PapyrusValue[]) => {
  const selfId = mp.getIdFromDesc(self.desc);
  const item = getObject(args, 0);
  const itemId = mp.getIdFromDesc(item.desc);
  const preventRemoval = args[1] ? getBoolean(args, 1) : false;
  const silent = args[2] ? getBoolean(args, 2) : false;

  const func = (ctx: Ctx, itemId: number, preventRemoval: boolean, silent: boolean) => {
    ctx.sp.once('update', () => {
      const form = ctx.sp.Game.getFormEx(itemId);
      ctx.sp.Game.getPlayer()?.unequipItem(form, preventRemoval, silent);
    });
  };
  evalClient(mp, selfId, new FunctionInfo(func).getText({ itemId, preventRemoval, silent }));

  if (!silent) {
    //notification
  }
};

export const unequipItemEx = (mp: Mp, self: PapyrusObject, args: PapyrusValue[]) => {
  const selfId = mp.getIdFromDesc(self.desc);
  const item = getObject(args, 0);
  const itemId = mp.getIdFromDesc(item.desc);
  const slot = args[1] ? getNumber(args, 1) : 0;
  const preventEquip = args[2] ? getBoolean(args, 2) : false;

  const func = (ctx: Ctx, itemId: number, slot: number, preventEquip: boolean) => {
    ctx.sp.once('update', () => {
      const form = ctx.sp.Game.getFormEx(itemId);
      ctx.sp.Game.getPlayer()?.unequipItemEx(form, slot, preventEquip);
    });
  };
  evalClient(mp, selfId, new FunctionInfo(func).getText({ itemId, slot, preventEquip }));
};

export const unequipAll = (mp: Mp, self: PapyrusObject, args: PapyrusValue[]) => {
  const selfId = mp.getIdFromDesc(self.desc);

  const func = (ctx: Ctx) => {
    ctx.sp.once('update', () => {
      ctx.sp.Game.getPlayer()?.unequipAll();
    });
  };
  evalClient(mp, selfId, new FunctionInfo(func).getText());
};

export const unequipItemSlot = (mp: Mp, self: PapyrusObject, args: PapyrusValue[]) => {
  const selfId = mp.getIdFromDesc(self.desc);
  const slotId = getNumber(args, 0);

  const func = (ctx: Ctx, slotId: number) => {
    ctx.sp.once('update', () => {
      ctx.sp.Game.getPlayer()?.unequipItemSlot(slotId);
    });
  };
  evalClient(mp, selfId, new FunctionInfo(func).getText({ slotId }));
};
