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

import { LocalizationProvider } from './src/utils/localizationProvider';
import * as fs from 'fs';
import * as path from 'path';

const config = JSON.parse(fs.readFileSync('server-settings.json', { encoding: 'utf-8' }));
const locale = config.locale;
const data = config.dataDir;
const isPapyrusHotReloadEnabled = config.isPapyrusHotReloadEnabled;

const localizationProvider = new LocalizationProvider(
  path.join(data, 'localization', locale + '.json'),
  isPapyrusHotReloadEnabled ? 'hotreload' : 'once'
);

perks.register(mp);

events.register(mp);

form.register(mp);
multiplayer.register(mp, localizationProvider);
stringUtil.register(mp);
actor.register(mp);
objectReference.register(mp);
utility.register(mp);
game.register(mp);

setTimeout(() => mp.callPapyrusFunction('global', 'GM_Main', '_OnPapyrusRegister', null, []), 0);
