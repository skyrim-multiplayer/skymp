import { Mp } from './src/types/mp';

declare const mp: Mp;

import * as multiplayer from './src/papyrus/multiplayer';
import * as events from './src/papyrus/events';
import * as stringUtil from './src/papyrus/stringUtil';
import * as actor from './src/papyrus/actor';
import * as objectReference from './src/papyrus/objectReference';
import * as utility from './src/papyrus/utility';
import * as game from './src/papyrus/game';
import * as form from './src/papyrus/form';

import * as perks from './src/properties/perks';
import * as avSkill from './src/properties/actorValues/skill';
import * as avAttributes from './src/properties/actorValues/attributes';
import * as evalProp from './src/properties/eval';
import * as browserProp from './src/properties/browser';

import { LocalizationProvider } from './src/utils/localizationProvider';

const config = mp.getServerSettings();
const locale = config.locale;
const isPapyrusHotReloadEnabled = config.isPapyrusHotReloadEnabled;

const localizationProvider = new LocalizationProvider(
  mp,
  'localization/' + locale + '.json',
  isPapyrusHotReloadEnabled ? 'hotreload' : 'once'
);

perks.register(mp);
evalProp.register(mp);
avSkill.register(mp);
avAttributes.register(mp);
browserProp.register(mp);

events.register(mp);

form.register(mp);
multiplayer.register(mp, localizationProvider);
stringUtil.register(mp);
actor.register(mp);
objectReference.register(mp);
utility.register(mp);
game.register(mp);

setTimeout(() => mp.callPapyrusFunction('global', 'GM_Main', '_OnPapyrusRegister', null, []), 0);
