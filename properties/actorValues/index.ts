import fs from 'fs';

import { utils } from '../../utility';
import { Attr, AttrRate, AttrRateMult, AttrDrain, AttrRelated, AttrAll, Modifier, MP, PropertyName } from '../../types';

declare const mp: MP;
export interface SecondsMatched {
	[key: number]: number;
}

export interface ActorValues {
	set: (formId: number, avName: AttrAll, modifierName: Modifier, newValue: any) => void;
	get: (formId: number, avName: AttrAll, modifierName: Modifier) => number;
	getMaximum: (formId: number, avName: Attr) => number;
	getCurrent: (formId: number, avName: Attr) => number;
	flushRegen: (formId: number, avName: Attr) => void;
	setDefaults: (formId: number, options: any) => void;
}
export interface AvOps {
	parent?: AvOps;
	set: (formId: number, avName: AttrAll, modifierName: Modifier, newValue: any) => void;
	get: (formId: number, avName: AttrAll, modifierName: Modifier) => number;
	applyRegenerationToParent?: (formId: number) => void;
	secondsMatched?: SecondsMatched;
	getSecondsMatched?: (formId: number) => number;
	setSecondsMatched?: (formId: number, secondsMatched: any) => void;
	multiplyDamage?: (formId: number, avName: AttrAll, k: number) => void;
}

const rate = (attr: Attr) => {
	return attr === 'health' ? 'av_healrate' : `av_${attr}rate`;
};
const mult = (attr: Attr) => {
	return attr === 'health' ? 'av_healratemult' : `av_${attr}ratemult`;
};
const drain = (attr: Attr) => {
	return `av_mp_${attr}drain`;
};

const updateAttributeCommon = (attr: Attr) => `
	const av = "${attr}";
	const ac = ctx.sp.Actor.from(ctx.refr);
	if (!ac) return;

	const base = ctx.value.base || 0;
	const perm = ctx.value.permanent || 0;
	const temp = ctx.value.temporary || 0;
	const targetMax = base + perm + temp;

	const numChangesKey = "${attr}NumChanges";
	const numChanges = ctx.get(numChangesKey);
	if (ctx.state[numChangesKey] !== numChanges) {
		ctx.state[numChangesKey] = numChanges;
		ctx.state.${attr}RegenStart = +Date.now();
	}

	const realTargetDmg = ctx.value.damage || 0;
	let targetDmg = realTargetDmg;

	if (av === "health" || ac.getFormId() == 0x14) {
		const multName = "${mult(attr)}";
		const rateName = "${rate(attr)}";
		const drainName = "${drain(attr)}";

		const additionalRegenMult = 1.0;
		const regenDuration = (+Date.now() - (ctx.state.${attr}RegenStart || 0)) / 1000;
		const healRateMult = ctx.get(multName);
		const healRateMultCurrent = (healRateMult.base || 0)
			+ (healRateMult.permanent || 0)
			+ (healRateMult.temporary || 0)
			+ (healRateMult.damage || 0);
		const healRate = ctx.get(rateName);
		const healRateCurrent = (healRate.base || 0)
			+ (healRate.permanent || 0)
			+ (healRate.temporary || 0)
			+ (healRate.damage || 0);

		const drain = ctx.get(drainName);
		const drainCurrent = (drain.base || 0)
			+ (drain.permanent || 0)
			+ (drain.temporary || 0)
			+ (drain.damage || 0);
		if (drainCurrent) {
			targetDmg += regenDuration * drainCurrent;
		}
		else {
			targetDmg += (regenDuration * additionalRegenMult
				* healRateCurrent * healRateMultCurrent * 0.01 * targetMax * 0.01);
		}

		if (targetDmg > 0) {
			targetDmg = 0;
		}
	}

	const currentPercentage = ac.getActorValuePercentage(av);
	const currentMax = ac.getBaseActorValue(av);

	let targetPercentage = (targetMax + targetDmg) / targetMax;
	if (ctx.get("isDead") && av === "health") {
		targetPercentage = 0;
	}

	const deltaPercentage = targetPercentage - currentPercentage;

	const k = (!targetPercentage || av === "stamina" || av === "magicka") ? 1 : 0.25;

	if (deltaPercentage > 0) {
		ac.restoreActorValue(av, deltaPercentage * currentMax * k);
	}
	else if (deltaPercentage < 0) {
		ac.damageActorValue(av, deltaPercentage * currentMax * k);
	}
`;

const updateAttributeNeighbor = (attr: Attr) => {
	return attr === 'health' ? updateAttributeCommon(attr) + `ac.setActorValue("${attr}", 9999);` : '';
};

const updateAttributeOwner = (attr: Attr) => {
	return updateAttributeCommon(attr) + `ac.setActorValue("${attr}", base);`;
};

const avs: AttrAll[] = [
	'healrate',
	'healratemult',
	'staminarate',
	'staminaratemult',
	'magickarate',
	'magickaratemult',
	'mp_healthdrain',
	'mp_magickadrain',
	'mp_staminadrain',
];
const relatedPropNames: AttrRelated[] = ['healthNumChanges', 'magickaNumChanges', 'staminaNumChanges'];

const getAvMaximum = (avOps: AvOps, formId: number, avName: AttrAll) => {
	let sum = 0;
	for (const modifierName of ['base', 'permanent', 'temporary'] as Modifier[]) {
		sum += avOps.get(formId, avName, modifierName);
	}
	return sum;
};

const getAvCurrent = (avOps: AvOps, formId: number, avName: AttrAll) => {
	let res = getAvMaximum(avOps, formId, avName);
	res += avOps.get(formId, avName, 'damage');
	return res;
};

// Regen
let regen = (
	avOps: AvOps,
	avNameTarget: Attr,
	avNameRate: AttrRate,
	avNameRateMult: AttrRateMult,
	avNameDrain: AttrDrain
): AvOps => {
	return {
		parent: avOps,
		set(formId: number, avName: AttrAll, modifierName: Modifier, newValue: any) {
			let dangerousAvNames = [avNameTarget, avNameRate, avNameRateMult, avNameDrain];
			dangerousAvNames = dangerousAvNames.map((x) => x.toLowerCase() as AttrAll);
			if (dangerousAvNames.indexOf(avName.toLowerCase() as AttrAll) !== -1 && this.applyRegenerationToParent) {
				this.applyRegenerationToParent(formId);
			}
			this.parent?.set(formId, avName, modifierName, newValue);
		},
		get(formId: number, avName: AttrAll, modifierName: Modifier) {
			if (!this.parent || !this.getSecondsMatched) {
				return 0;
			}

			const drain = getAvCurrent(this.parent, formId, avNameDrain);
			const realValue = this.parent.get(formId, avName, modifierName);
			if (avName.toLowerCase() === avNameTarget.toLowerCase()) {
				if (modifierName === 'damage') {
					const avMax = getAvMaximum(this.parent, formId, avName);
					const regenDuration = timeSource.getSecondsPassed() - this.getSecondsMatched(formId);
					const rate = getAvCurrent(this.parent, formId, avNameRate);
					const rateMult = getAvCurrent(this.parent, formId, avNameRateMult);
					let damageMod = realValue;
					if (drain) {
						damageMod += regenDuration * drain;
					} else {
						damageMod += regenDuration * rate * rateMult * 0.01 * avMax * 0.01;
					}
					return Math.min(0, damageMod);
				}
			}
			return realValue;
		},
		getSecondsMatched(formId: number) {
			return (this.secondsMatched && this.secondsMatched[formId]) || 0;
		},
		setSecondsMatched(formId: number, secondsMatched: any) {
			if (!this.secondsMatched) {
				this.secondsMatched = {};
			}
			this.secondsMatched[formId] = secondsMatched;
		},
		applyRegenerationToParent(formId: number) {
			// ? Не уверен в проверке !this.getSecondsMatched
			if (!this.parent || !this.setSecondsMatched) {
				return 0;
			}

			const damageAfterRegen = this.get(formId, avNameTarget, 'damage');
			this.parent.set(formId, avNameTarget, 'damage', damageAfterRegen);
			this.setSecondsMatched(formId, timeSource.getSecondsPassed());
		},
	};
};

const timeSource = {
	startMoment: Date.now(),
	getSecondsPassed(): number {
		if (!this.startMoment) {
			this.startMoment = Date.now();
		}
		return (+Date.now() - +this.startMoment) / 1000.0;
	},
};

export let actorValues: ActorValues;

export const initActorValue = () => {
	for (const attr of ['health', 'magicka', 'stamina'] as Attr[]) {
		mp.makeProperty(('av_' + attr) as PropertyName, {
			isVisibleByOwner: true,
			isVisibleByNeighbors: attr === 'health',
			updateNeighbor: updateAttributeNeighbor(attr),
			updateOwner: updateAttributeOwner(attr),
		});
	}

	for (const avName of avs) {
		mp.makeProperty(('av_' + avName) as PropertyName, {
			isVisibleByOwner: true,
			isVisibleByNeighbors: true,
			updateNeighbor: '',
			updateOwner: '',
		});
	}

	for (const propName of relatedPropNames) {
		mp.makeProperty(propName, {
			isVisibleByOwner: true,
			isVisibleByNeighbors: true,
			updateNeighbor: '',
			updateOwner: '',
		});
	}

	// Basic
	let avOps: AvOps = {
		set(formId: number, avName: AttrAll, modifierName: Modifier, newValue: number) {
			const propName = ('av_' + avName.toLowerCase()) as PropertyName;
			const value = mp.get(formId, propName);
			value[modifierName] = newValue;
			mp.set(formId, propName, value);
			if (['health', 'magicka', 'stamina'].indexOf(avName.toLowerCase()) !== -1) {
				const propName = `${avName.toLowerCase()}NumChanges` as PropertyName;
				mp.set(formId, propName, 1 + (mp.get(formId, propName) || 0));
			}
		},
		get(formId: number, avName: AttrAll, modifierName: Modifier) {
			const propName = ('av_' + avName.toLowerCase()) as PropertyName;
			const propValue = mp.get(formId, propName);
			if (propValue === undefined) {
				const s = `'${propName}' was '${propValue}' for ${formId.toString(16)}`;
				throw new Error(s);
			}
			return propValue[modifierName] || 0;
		},
	};

	// Damage limit
	avOps = {
		parent: avOps,
		set(formId: number, avName: AttrAll, modifierName: Modifier, newValue: number) {
			if (!this.parent) {
				return;
			}

			if (modifierName == 'damage') {
				if (newValue > 0) {
					newValue = 0;
				} else if (newValue < -getAvMaximum(this.parent, formId, avName)) {
					newValue = -getAvMaximum(this.parent, formId, avName);
				}
			}
			this.parent.set(formId, avName, modifierName, newValue);
		},
		get(formId: number, avName: AttrAll, modifierName: Modifier) {
			if (!this.parent) {
				return 0;
			}
			return this.parent.get(formId, avName, modifierName);
		},
	};

	avOps = regen(avOps, 'health', 'healrate', 'healratemult', 'mp_healthdrain');
	avOps = regen(avOps, 'magicka', 'magickarate', 'magickaratemult', 'mp_magickadrain');
	avOps = regen(avOps, 'stamina', 'staminarate', 'staminaratemult', 'mp_staminadrain');

	// Scaling
	avOps = {
		parent: avOps,
		set(formId: number, avName: AttrAll, modifierName: Modifier, newValue: any) {
			if (!this.parent) {
				return;
			}

			let oldMaximum: number, newMaximum: number;

			oldMaximum = getAvMaximum(this.parent, formId, avName);
			this.parent.set(formId, avName, modifierName, newValue);
			newMaximum = getAvMaximum(this.parent, formId, avName);

			const k = newMaximum / oldMaximum;
			if (isFinite(k) && k != 1 && this.multiplyDamage) {
				this.multiplyDamage(formId, avName, k);
			}
		},
		get(formId: number, avName: AttrAll, modifierName: Modifier) {
			// ? Не уверен в проверке !this.getSecondsMatched
			if (!this.parent) {
				return 0;
			}
			return this.parent.get(formId, avName, modifierName);
		},
		multiplyDamage(formId: number, avName: AttrAll, k: number) {
			if (!this.parent) {
				return;
			}
			const previousDamage: number = this.parent.get(formId, avName, 'damage');
			this.parent.set(formId, avName, 'damage', previousDamage * k);
		},
	};

	actorValues = {
		set: (formId: number, avName: AttrAll, modifierName: Modifier, newValue: any) =>
			avOps.set(formId, avName, modifierName, newValue),
		get: (formId: number, avName: AttrAll, modifierName: Modifier) => avOps.get(formId, avName, modifierName),
		getMaximum: (formId: number, avName: Attr) => getAvMaximum(avOps, formId, avName),
		getCurrent: (formId: number, avName: Attr) => getAvCurrent(avOps, formId, avName),
		flushRegen: (formId: number, avName: Attr) => {
			const damageModAfterRegen = avOps.get(formId, avName, 'damage');
			avOps.set(formId, avName, 'damage', damageModAfterRegen);
		},
		setDefaults: (formId: number, options: any) => {
			const force = options && options.force;
			if (utils.isActor(formId)) {
				if (mp.get(formId, 'isDead') === undefined || force) {
					mp.set(formId, 'isDead', false);
				}
				for (const avName of ['health', 'magicka', 'stamina']) {
					if (!mp.get(formId, ('av_' + avName) as PropertyName) || force) {
						mp.set(formId, ('av_' + avName) as PropertyName, { base: 100 });
					}
				}
				for (const avName of ['healrate', 'magickarate', 'staminarate']) {
					if (!mp.get(formId, ('av_' + avName) as PropertyName) || force) {
						mp.set(formId, ('av_' + avName) as PropertyName, { base: 5 });
					}
				}
				for (const avName of ['healratemult', 'magickaratemult', 'staminaratemult']) {
					if (!mp.get(formId, ('av_' + avName) as PropertyName) || force) {
						mp.set(formId, ('av_' + avName) as PropertyName, { base: 100 });
					}
				}
				for (const avName of ['mp_healthdrain', 'mp_magickadrain', 'mp_staminadrain']) {
					//if (formId == 0x9b7a2) console.log(avName, 1);
					if (!mp.get(formId, ('av_' + avName) as PropertyName) || force) {
						//if (formId == 0x9b7a2) console.log(avName, 2);
						mp.set(formId, ('av_' + avName) as PropertyName, { base: 0 });
						if (formId == 0x9b7a2) fs.writeFileSync('kekekek', '' + mp.get(formId, ('av_' + avName) as PropertyName));
						if (formId == 0x9b7a2) {
							setTimeout(() => {
								fs.writeFileSync('kekekek1', '' + mp.get(formId, ('av_' + avName) as PropertyName));
							}, 100);
						}
					}
				}
			}
		},
	};

	utils.hook('onReinit', (pcFormId: number, options: any) => {
		/** set default value to Actor */
		if (actorValues.setDefaults) {
			actorValues.setDefaults(pcFormId, options);
		}
	});
};
