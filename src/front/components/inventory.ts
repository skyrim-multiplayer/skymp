import {
  getExtraContainerChanges,
  getContainer,
  BaseExtraList,
  ExtraHealth,
  ObjectReference,
  TESModPlatform,
  Game,
  printConsole,
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
} from "skyrimPlatform";

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

// 'loxsword (Legendary)' => 'loxsword'
const getRealName = (s?: string): string => {
  if (!s) return s;

  const arr = s.split(" ");
  if (arr.length && arr[arr.length - 1].match(/^\(.*\)$/)) arr.pop();
  return arr.join(" ");
};

// 'aaaaaaaaaaaaaaaa' => 'aaa...'
const cropName = (s?: string): string => {
  if (!s) return s;

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

const extrasEqual = (a: Entry, b: Entry) => {
  return (
    a.health === b.health &&
    a.enchantmentId === b.enchantmentId &&
    a.maxCharge === b.maxCharge &&
    !!a.removeEnchantmentOnUnequip === !!b.removeEnchantmentOnUnequip &&
    a.chargePercent === b.chargePercent &&
    namesEqual(a, b) &&
    a.soul === b.soul &&
    a.poisonId === b.poisonId &&
    a.poisonCount === b.poisonCount &&
    !!a.worn === !!b.worn &&
    !!a.wornLeft === !!b.wornLeft
  );
};

const hasExtras = (e: Entry): boolean => {
  return !extrasEqual(e, { baseId: 0, count: 0 });
};

export interface Inventory {
  entries: Entry[];
}

const extractExtraData = (
  extraList: BaseExtraList | null,
  out: Entry
): void => {
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
        out.removeEnchantmentOnUnequip = (extra as ExtraEnchantment).removeOnUnequip;
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

const getExtraContainerChangesAsInventory = (
  refr: ObjectReference
): Inventory => {
  const extraContainerChanges = getExtraContainerChanges(refr.getFormID());
  const entries = new Array<Entry>();

  extraContainerChanges.forEach((changesEntry) => {
    const entry: Entry = {
      baseId: changesEntry.baseId,
      count: changesEntry.countDelta,
    };

    (changesEntry.extendDataList || []).forEach((extraList) => {
      const e: Entry = {
        baseId: entry.baseId,
        count: 1,
      };
      extractExtraData(extraList, e);
      entries.push(e);
      entry.count -= e.count;
    });

    if (entry.count !== 0) entries.push(entry);
  });
  return { entries };
};

const getBaseContainerAsInventory = (refr: ObjectReference): Inventory => {
  return { entries: getContainer(refr.getBaseObject().getFormID()) };
};

const sumInventories = (lhs: Inventory, rhs: Inventory): Inventory => {
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

const getDiff = (lhs: Inventory, rhs: Inventory): Inventory => {
  const lhsCopy: Inventory = JSON.parse(JSON.stringify(lhs));
  const rhsCopy: Inventory = JSON.parse(JSON.stringify(rhs));

  rhsCopy.entries.forEach((e) => {
    const sameFromLeft = lhsCopy.entries.find(
      (x) => x.baseId === e.baseId && extrasEqual(x, e)
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
  return storage["basesReset"];
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
  newInventory: Inventory
): void => {
  resetBase(refr);
  const diff = getDiff(newInventory, getInventory(refr)).entries;
  if (diff.length) printConsole("diff " + diff.length);
  diff.sort((a, b) => (a.count < b.count ? -1 : 1));
  diff.forEach((e) => {
    const absCount = Math.abs(e.count);

    let queueNiNodeUpdateNeeded = false;

    printConsole(e);

    const worn = !!e.worn;
    const wornLeft = !!e.wornLeft;

    /*for (let i = 0; i < absCount; ++i) {
      if (worn || wornLeft) {
        if (e.count < 0) {
          const ac = Actor.from(refr);
          const f = Game.getFormEx(e.baseId);
          if (ac && f) {
            ac.unequipItem(f, false, true);
          }
        }
      }
    }*/

    let k = 0;
    for (let i = 0; i < absCount; ++i) {
      k++;
      if (k > 1000) {
        printConsole("huh!", absCount);
        break;
      }

      if (worn || wornLeft) {
        TESModPlatform.pushWornState(!!worn, !!wornLeft);
        queueNiNodeUpdateNeeded = true;
      }

      TESModPlatform.addItemEx(
        refr,
        Game.getFormEx(e.baseId),
        e.count / absCount,
        e.health ? e.health : 1,
        e.enchantmentId
          ? Enchantment.from(Game.getFormEx(e.enchantmentId))
          : null,
        e.maxCharge ? e.maxCharge : 0,
        !!e.removeEnchantmentOnUnequip,
        e.chargePercent ? e.chargePercent : 0,
        e.name ? cropName(e.name) : "",
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
};
