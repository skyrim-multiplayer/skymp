import { MpApiInteractor } from './mpApiInteractor';
import { BrowserProperty } from './src/props/browserProperty';
import { ChatProperty } from './src/props/chatProperty';
import { DialogProperty } from './src/props/dialogProperty';
import { EvalProperty } from './src/props/evalProperty';
import { SweetPieGameModeListener, SweetPieMap } from './src/sweetPie';
import { LocationalData, Mp } from './src/types/mp';
import { Timer } from './src/utils/timer';

DialogProperty.init();
BrowserProperty.init();
EvalProperty.init();
ChatProperty.init();
Timer.init();

console.log('gamemode.js reloaded');

const pointsByName = new Map<string, LocationalData>();
pointsByName.set('hall:spawnPoint', {
  pos: [18511, 10256, 610.6392],
  cellOrWorldDesc: '42b5f:SweetPie.esp',
  rot: [0, 0, 347],
});
pointsByName.set('whiterun:spawnPoint', {
  pos: [22659, -8697, -3594],
  cellOrWorldDesc: '1a26f:Skyrim.esm',
  rot: [0, 0, 268],
});
pointsByName.set('whiterun:safePlace', {
  pos: [-108, -809, 69.25],
  cellOrWorldDesc: '1605e:Skyrim.esm',
  rot: [0, 0, 176],
});

const maps: Required<SweetPieMap>[] = [{
  safePointName: 'whiterun:safePlace',
  mainSpawnPointName: 'whiterun:spawnPoint',
  safePlaceEnterDoors: ['1a6f4:Skyrim.esm'],
  safePlaceLeaveDoors: ['16072:Skyrim.esm'],
  leaveRoundDoors: ['1b1f3:Skyrim.esm']
}];

const playerController = MpApiInteractor.makeController(pointsByName);
const gameModeListener = new SweetPieGameModeListener(playerController, maps);
MpApiInteractor.setup(gameModeListener);