import {
  getExtraContainerChanges,
  getContainer,
  BaseExtraList,
  ExtraHealth,
  ObjectReference,
  TESModPlatform,
  Game,
  storage,
  ExtraCount,
  ExtraEnchantment,
  ExtraPoison,
  ExtraSoul,
  ExtraTextDisplayData,
  ExtraCharge,
  Enchantment,
  Potion,
  Actor,
  Ammo,
  printConsole,
  ActorBase,
  FormType,
} from "skyrimPlatform";
import * as taffyPerkSystem from "../sweetpie/taffyPerkSystem";

export interface Extra {
  health?: number;
  enchantmentId?: number;
  maxCharge?: number;
  removeEnchantmentOnUnequip?: boolean;
  chargePercent?: number;
  name?: string;
  soul?: 0 | 1 | 2 | 3 | 4 | 5;
  poisonId?: number;
  poisonCount?: number;
  worn?: boolean;
  wornLeft?: boolean;
}

export interface BasicEntry {
  baseId: number;
  count: number;
}

export type Entry = BasicEntry & Extra;

export interface Inventory {
  entries: Entry[];
}

// 'loxsword (Legendary)' => 'loxsword'
const getRealName = (s?: string): string => {
  if (!s) return s as string;

  const arr = s.split(" ");
  if (arr.length && arr[arr.length - 1].match(/^\(.*\)$/)) arr.pop();
  return arr.join(" ");
};

// 'aaaaaaaaaaaaaaaa' => 'aaa...'
const cropName = (s?: string): string => {
  if (!s) return s as string;

  const max = 128;
  return s.length >= max
    ? s
        .split("")
        .filter((x, i) => i < max)
        .join("")
        .concat("...")
    : s;
};

const checkIfNameIsGeneratedByGame = (
  aStr: string,
  bStr: string,
  formName: string
) => {
  if (!aStr.length && bStr.startsWith(formName)) {
    const bEnding = bStr.substr(formName.length);
    if (bEnding.match(/^\s\(.*\)$/)) {
      return true;
    }
  }
  return false;
};

const namesEqual = (a: Entry, b: Entry): boolean => {
  const aStr = a.name || "";
  const bStr = b.name || "";
  if (cropName(getRealName(aStr)) === cropName(getRealName(bStr))) return true;

  if (a.baseId === b.baseId) {
    const form = Game.getFormEx(a.baseId);
    if (form) {
      const formName = form.getName();
      if (
        checkIfNameIsGeneratedByGame(aStr, bStr, formName) ||
        checkIfNameIsGeneratedByGame(bStr, aStr, formName)
      )
        return true;
    }
  }

  return false;
};

const extrasEqual = (a: Entry, b: Entry, ignoreWorn = false) => {
  return (
    a.health === b.health &&
    a.enchantmentId === b.enchantmentId &&
    a.maxCharge === b.maxCharge &&
    !!a.removeEnchantmentOnUnequip === !!b.removeEnchantmentOnUnequip &&
    a.chargePercent === b.chargePercent &&
    //namesEqual(a, b) &&
    a.soul === b.soul &&
    a.poisonId === b.poisonId &&
    a.poisonCount === b.poisonCount &&
    ((!!a.worn === !!b.worn && !!a.wornLeft === !!b.wornLeft) || ignoreWorn)
  );
};

export const hasExtras = (e: Entry): boolean => {
  return !extrasEqual(e, { baseId: 0, count: 0 });
};

const extractExtraData = (
  refr: ObjectReference,
  extraList: BaseExtraList | null,
  out: Entry
): void => {
  // I see that ExtraWorn is not emitted for 0xFF actors when arrows are equipped. Fixing
  const item = Game.getFormEx(out.baseId);
  if (Ammo.from(item)) {
    const actor = Actor.from(refr);
    if (actor && actor.isEquipped(item)) {
      out.worn = true;
    }
  }

  (extraList || []).forEach((extra) => {
    switch (extra.type) {
      case "Health":
        out.health = Math.round((extra as ExtraHealth).health * 10) / 10;

        // TESModPlatform::AddItemEx makes all items at least 1.01 health
        if (out.health === 1) {
          delete out.health;
        }
        break;
      case "Count":
        out.count = (extra as ExtraCount).count;
        break;
      case "Enchantment":
        out.enchantmentId = (extra as ExtraEnchantment).enchantmentId;
        out.maxCharge = (extra as ExtraEnchantment).maxCharge;
        out.removeEnchantmentOnUnequip = (
          extra as ExtraEnchantment
        ).removeOnUnequip;
        break;
      case "Charge":
        out.chargePercent = (extra as ExtraCharge).charge;
        break;
      case "Poison":
        out.poisonId = (extra as ExtraPoison).poisonId;
        out.poisonCount = (extra as ExtraPoison).count;
        break;
      case "Soul":
        out.soul = (extra as ExtraSoul).soul;
        break;
      case "TextDisplayData":
        out.name = (extra as ExtraTextDisplayData).name;
        break;
      case "Worn":
        out.worn = true;
        break;
      case "WornLeft":
        out.wornLeft = true;
        break;
    }
  });
};

const squash = (inv: Inventory): Inventory => {
  const res = new Array<Entry>();
  inv.entries.forEach((e) => {
    const same = res.find((x) => e.baseId === x.baseId && extrasEqual(x, e));
    if (same) {
      same.count += e.count;
    } else {
      res.push(JSON.parse(JSON.stringify(e)));
    }
  });
  return { entries: res.filter((x) => x.count !== 0) };
};

const getExtraContainerChangesAsInventory = (
  refr: ObjectReference
): Inventory => {
  const extraContainerChanges = getExtraContainerChanges(refr.getFormID());
  const entries = new Array<Entry>();

  (extraContainerChanges || []).forEach((changesEntry) => {
    const entry: Entry = {
      baseId: changesEntry.baseId,
      count: changesEntry.countDelta,
    };

    (changesEntry.extendDataList || []).forEach((extraList) => {
      const e: Entry = {
        baseId: entry.baseId,
        count: 1,
      };
      extractExtraData(refr, extraList, e);
      entries.push(e);
      entry.count -= e.count;
    });

    if (entry.count !== 0) entries.push(entry);
  });

  let res: Inventory = { entries };
  res = squash(res);
  return res;
};

const getBaseContainerAsInventory = (refr: ObjectReference): Inventory => {
  return {
    entries: getContainer((refr.getBaseObject() as ActorBase).getFormID()),
  };
};

export const sumInventories = (lhs: Inventory, rhs: Inventory): Inventory => {
  const leftEntriesWithExtras = lhs.entries.filter((e) => hasExtras(e));
  const rightEntriesWithExtras = rhs.entries.filter((e) => hasExtras(e));
  const leftEntriesSimple = lhs.entries.filter((e) => !hasExtras(e));
  const rightEntriesSimple = rhs.entries.filter((e) => !hasExtras(e));

  leftEntriesSimple.forEach((e) => {
    const matching = rightEntriesSimple.find((x) => x.baseId === e.baseId);
    if (matching) {
      e.count += matching.count;
      matching.count = 0;
    }
  });

  return {
    entries: leftEntriesWithExtras
      .concat(rightEntriesWithExtras)
      .concat(leftEntriesSimple)
      .concat(rightEntriesSimple)
      .filter((e) => e.count !== 0),
  };
};

export const removeSimpleItemsAsManyAsPossible = (
  inv: Inventory,
  baseId: number,
  count: number
): Inventory => {
  const res: Inventory = { entries: [] };
  res.entries = JSON.parse(JSON.stringify(inv.entries));

  const entry = res.entries.find((e) => !hasExtras(e) && e.baseId === baseId);
  if (entry) {
    entry.count -= count;
  }

  res.entries = res.entries.filter((e) => e.count > 0);
  return res;
};

export const getDiff = (
  lhs: Inventory,
  rhs: Inventory,
  ignoreWorn: boolean
): Inventory => {
  const lhsCopy: Inventory = JSON.parse(JSON.stringify(lhs));
  const rhsCopy: Inventory = JSON.parse(JSON.stringify(rhs));

  rhsCopy.entries.forEach((e) => {
    const sameFromLeft = lhsCopy.entries.find(
      (x) => x.baseId === e.baseId && extrasEqual(x, e, ignoreWorn)
    );
    if (sameFromLeft) {
      sameFromLeft.count -= e.count;
    } else {
      lhsCopy.entries.push(e);
      lhsCopy.entries[lhsCopy.entries.length - 1].count *= -1;
    }
  });

  return { entries: lhsCopy.entries.filter((x) => x.count !== 0) };
};

export const getInventory = (refr: ObjectReference): Inventory => {
  return squash(
    sumInventories(
      getBaseContainerAsInventory(refr),
      getExtraContainerChangesAsInventory(refr)
    )
  );
};

const basesReset = (): Set<number> => {
  if (storage["basesResetExists"] !== true) {
    storage["basesResetExists"] = true;
    storage["basesReset"] = new Set<number>();
  }
  return storage["basesReset"] as Set<number>;
};

const resetBase = (refr: ObjectReference): void => {
  const base = refr.getBaseObject();
  const baseId = base ? base.getFormID() : 0;
  if (!basesReset().has(baseId)) {
    basesReset().add(baseId);
    TESModPlatform.resetContainer(base);

    refr.removeAllItems(null, false, true);
  }
};

export const applyInventory = (
  refr: ObjectReference,
  newInventory: Inventory,
  enableCrashProtection: boolean,
  ignoreWorn = false
): boolean => {
  resetBase(refr);
  const diff = getDiff(newInventory, getInventory(refr), ignoreWorn).entries;

  let res = true;

  diff.sort((a, b) => (a.count < b.count ? -1 : 1));
  diff.forEach((e, i) => {
    taffyPerkSystem.inventoryChanged(refr, e);

    if (i > 0 && enableCrashProtection) {
      res = false;
      return;
    }
    let absCount = Math.abs(e.count);

    let queueNiNodeUpdateNeeded = false;

    const worn = !!e.worn;
    const wornLeft = !!e.wornLeft;

    let oneStepCount = e.count / absCount;

    const f = Game.getFormEx(e.baseId);
    if (!f) {
      return printConsole(`Bad form ID ${e.baseId.toString(16)}`);
    }
    const type = f.getType();
    // For misc items, potions and ingredients we don't want to split them into multiple items
    // This was made to fix a performance issue with users having 10000+ of misc items (i.e. gold)
    if (
      type === FormType.Misc ||
      type === FormType.Potion ||
      type === FormType.Ingredient
    ) {
      absCount = 1;
      oneStepCount = e.count;
    } else {
      if (absCount > 1000) {
        absCount = 1;
        oneStepCount = 1;

        // Also for arrows with strange count
        if (worn && e.count < 0) {
          absCount = 0;
        }
      }

      if (e.count > 1 && Ammo.from(Game.getFormEx(e.baseId))) {
        absCount = 1;
        oneStepCount = e.count;
        if (e.count > 60000) {
          // Why would actor have 60k arrows?
          e.count = 1;
        }
      }
    }

    for (let i = 0; i < absCount; ++i) {
      if (worn || wornLeft) {
        TESModPlatform.pushWornState(!!worn, !!wornLeft);
        queueNiNodeUpdateNeeded = true;
      }

      printConsole(
        `Adding ${e.baseId} to ${refr
          .getFormID()
          .toString(16)} with count ${oneStepCount}`
      );
      TESModPlatform.addItemEx(
        refr,
        f,
        oneStepCount,
        e.health ? e.health : 1,
        e.enchantmentId
          ? Enchantment.from(Game.getFormEx(e.enchantmentId))
          : null,
        e.maxCharge ? e.maxCharge : 0,
        !!e.removeEnchantmentOnUnequip,
        e.chargePercent ? e.chargePercent : 0,
        e.name ? cropName(e.name) : f.getName(),
        e.soul ? e.soul : 0,
        e.poisonId ? Potion.from(Game.getFormEx(e.poisonId)) : null,
        e.poisonCount ? e.poisonCount : 0
      );
    }

    if (queueNiNodeUpdateNeeded) {
      const ac = Actor.from(refr);
      if (ac) {
        ac.queueNiNodeUpdate();
      }
    }
  });

  return res;
};
