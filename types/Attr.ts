export type Attr = 'health' | 'magicka' | 'stamina';

export type AttrUppercase = 'Health' | 'Magicka' | 'Stamina';

export type AttrRate = 'healrate' | 'magickarate' | 'staminarate';

export type AttrRateMult =
	| 'healratemult'
	| 'magickaratemult'
	| 'staminaratemult';

export type AttrDrain =
	| 'mp_healthdrain'
	| 'mp_magickadrain'
	| 'mp_staminadrain';

export type AttrRelated =
	| 'healthNumChanges'
	| 'magickaNumChanges'
	| 'staminaNumChanges';

export type AttrAll = Attr | AttrRate | AttrRateMult | AttrDrain;
