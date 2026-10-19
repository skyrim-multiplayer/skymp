export type ItemCategory =
  | 'weapons'
  | 'armor'
  | 'clothing'
  | 'potions'
  | 'food'
  | 'ingredients'
  | 'misc';

export interface CatalogItem {
  codeHex: string;
  name: string;
  category: ItemCategory;
}

// Curated starter set. Keep manual code input available as fallback for missing items.
export const ITEM_CATALOG: CatalogItem[] = [
  // Weapons
  { codeHex: '0001397E', name: 'Iron Dagger', category: 'weapons' },
  { codeHex: '0001397D', name: 'Iron Sword', category: 'weapons' },
  { codeHex: '00013981', name: 'Iron War Axe', category: 'weapons' },
  { codeHex: '00013980', name: 'Iron Mace', category: 'weapons' },
  { codeHex: '00013982', name: 'Iron Greatsword', category: 'weapons' },
  { codeHex: '00013983', name: 'Iron Battleaxe', category: 'weapons' },
  { codeHex: '00013984', name: 'Iron Warhammer', category: 'weapons' },
  { codeHex: '0001397F', name: 'Iron Bow', category: 'weapons' },
  { codeHex: '00013985', name: 'Steel Dagger', category: 'weapons' },
  { codeHex: '00013989', name: 'Steel Sword', category: 'weapons' },
  { codeHex: '0001398B', name: 'Steel Mace', category: 'weapons' },
  { codeHex: '0001398A', name: 'Steel War Axe', category: 'weapons' },
  { codeHex: '00013987', name: 'Steel Greatsword', category: 'weapons' },
  { codeHex: '00013988', name: 'Steel Battleaxe', category: 'weapons' },
  { codeHex: '0001398C', name: 'Steel Warhammer', category: 'weapons' },
  { codeHex: '00013986', name: 'Hunting Bow', category: 'weapons' },
  { codeHex: '000139A8', name: 'Glass Bow', category: 'weapons' },
  { codeHex: '000139A5', name: 'Glass Greatsword', category: 'weapons' },
  { codeHex: '000139A7', name: 'Glass Sword', category: 'weapons' },
  { codeHex: '000139AD', name: 'Ebony Bow', category: 'weapons' },
  { codeHex: '000139AB', name: 'Ebony War Axe', category: 'weapons' },
  { codeHex: '000139AA', name: 'Ebony Sword', category: 'weapons' },
  { codeHex: '000139AF', name: 'Daedric Battleaxe', category: 'weapons' },
  { codeHex: '000139B1', name: 'Daedric Sword', category: 'weapons' },
  { codeHex: '000139B5', name: 'Daedric Bow', category: 'weapons' },
  { codeHex: '000139B6', name: 'Daedric Dagger', category: 'weapons' },
  { codeHex: '000139B9', name: 'Daedric Greatsword', category: 'weapons' },
  { codeHex: '000139B2', name: 'Daedric War Axe', category: 'weapons' },
  { codeHex: '000139C0', name: 'Daedric Mace', category: 'weapons' },
  { codeHex: '000139B0', name: 'Daedric Warhammer', category: 'weapons' },

  // Armor
  { codeHex: '00012EB7', name: 'Iron Armor', category: 'armor' },
  { codeHex: '00012E46', name: 'Iron Helmet', category: 'armor' },
  { codeHex: '00012E4D', name: 'Iron Gauntlets', category: 'armor' },
  { codeHex: '00012E4F', name: 'Iron Boots', category: 'armor' },
  { codeHex: '00012EB6', name: 'Iron Shield', category: 'armor' },
  { codeHex: '00013952', name: 'Dragonplate Armor', category: 'armor' },
  { codeHex: '0001393E', name: 'Dragonbone Armor', category: 'armor' },
  { codeHex: '00013939', name: 'Glass Armor', category: 'armor' },
  { codeHex: '0001393B', name: 'Glass Boots', category: 'armor' },
  { codeHex: '0001393A', name: 'Glass Gauntlets', category: 'armor' },
  { codeHex: '00013938', name: 'Glass Helmet', category: 'armor' },
  { codeHex: '00013941', name: 'Ebony Armor', category: 'armor' },
  { codeHex: '00013942', name: 'Ebony Boots', category: 'armor' },
  { codeHex: '00013943', name: 'Ebony Gauntlets', category: 'armor' },
  { codeHex: '00013940', name: 'Ebony Helmet', category: 'armor' },
  { codeHex: '00013964', name: 'Ebony Shield', category: 'armor' },
  { codeHex: '0001396B', name: 'Daedric Armor', category: 'armor' },
  { codeHex: '0001396A', name: 'Daedric Boots', category: 'armor' },
  { codeHex: '0001396D', name: 'Daedric Gauntlets', category: 'armor' },
  { codeHex: '0001396E', name: 'Daedric Helmet', category: 'armor' },
  { codeHex: '0001396C', name: 'Daedric Shield', category: 'armor' },

  // Clothing
  { codeHex: '000D191F', name: 'Fine Clothes', category: 'clothing' },
  { codeHex: '00086983', name: 'Farm Clothes', category: 'clothing' },
  { codeHex: '0009B11A', name: 'Noble Clothes', category: 'clothing' },
  { codeHex: '000261C0', name: 'Chef Hat', category: 'clothing' },
  { codeHex: '000D1920', name: 'Fine Boots', category: 'clothing' },
  { codeHex: '000D1922', name: 'Fine Hat', category: 'clothing' },
  { codeHex: '000D191D', name: 'Merchant Clothes', category: 'clothing' },
  {
    codeHex: '000D1923',
    name: 'Radiant Raiment Fine Clothes',
    category: 'clothing'
  },
  { codeHex: '000CEE80', name: 'College Boots', category: 'clothing' },

  // Potions
  { codeHex: '00039BE5', name: 'Potion of Minor Healing', category: 'potions' },
  { codeHex: '00039BE4', name: 'Potion of Healing', category: 'potions' },
  {
    codeHex: '00039BE3',
    name: 'Potion of Plentiful Healing',
    category: 'potions'
  },
  {
    codeHex: '00039BE2',
    name: 'Potion of Vigorous Healing',
    category: 'potions'
  },
  { codeHex: '00039BE0', name: 'Potion of Minor Magicka', category: 'potions' },
  { codeHex: '00039BDF', name: 'Potion of Magicka', category: 'potions' },
  { codeHex: '00039BE8', name: 'Potion of Minor Stamina', category: 'potions' },
  { codeHex: '00039BE7', name: 'Potion of Stamina', category: 'potions' },
  { codeHex: '000EA5C8', name: 'Philter of Health', category: 'potions' },
  { codeHex: '000EA5D2', name: 'Philter of Stamina', category: 'potions' },
  { codeHex: '000EA5CA', name: 'Philter of Magicka', category: 'potions' },

  // Food
  { codeHex: '00064B3F', name: 'Bread', category: 'food' },
  { codeHex: '00064B41', name: 'Cheese Wedge', category: 'food' },
  { codeHex: '00064B31', name: 'Beef', category: 'food' },
  { codeHex: '00064B34', name: 'Cabbage', category: 'food' },
  { codeHex: '00064B35', name: 'Carrot', category: 'food' },
  { codeHex: '00064B3C', name: 'Leek', category: 'food' },
  { codeHex: '00064B3D', name: 'Potato', category: 'food' },
  { codeHex: '00064B36', name: 'Apple Pie', category: 'food' },
  { codeHex: '00064B43', name: 'Ale', category: 'food' },

  // Ingredients
  {
    codeHex: '0003AD66',
    name: 'Blue Mountain Flower',
    category: 'ingredients'
  },
  { codeHex: '0003AD60', name: 'Wheat', category: 'ingredients' },
  { codeHex: '0006ABCB', name: 'Salt Pile', category: 'ingredients' },
  { codeHex: '00077E1C', name: 'Garlic', category: 'ingredients' },
  { codeHex: '0003AD61', name: 'Canis Root', category: 'ingredients' },
  { codeHex: '0003AD5F', name: 'Snowberries', category: 'ingredients' },
  { codeHex: '00063B5F', name: 'Jazbay Grapes', category: 'ingredients' },

  // Misc
  { codeHex: '0000000F', name: 'Gold', category: 'misc' },
  { codeHex: '0005ACE4', name: 'Lockpick', category: 'misc' },
  { codeHex: '0001D4EC', name: 'Torch', category: 'misc' },
  { codeHex: '00063B27', name: 'Soul Gem (Petty)', category: 'misc' },
  { codeHex: '0002E4FF', name: 'Soul Gem (Common)', category: 'misc' },
  { codeHex: '0002E500', name: 'Soul Gem (Greater)', category: 'misc' },
  { codeHex: '0002E501', name: 'Soul Gem (Grand)', category: 'misc' },
  { codeHex: '0000009F', name: 'Leather', category: 'misc' },
  { codeHex: '000DB5D2', name: 'Leather Strips', category: 'misc' },
  { codeHex: '0005AD9E', name: 'Firewood', category: 'misc' }
];

export const ITEM_CATEGORIES: ItemCategory[] = [
  'weapons',
  'armor',
  'clothing',
  'potions',
  'food',
  'ingredients',
  'misc'
];
