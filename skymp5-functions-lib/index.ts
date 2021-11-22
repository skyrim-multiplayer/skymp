import { BrowserProperty } from './src/props/browserProperty';
import { DialogProperty } from './src/props/dialogProperty';
import { EvalProperty } from './src/props/evalProperty';
import { Ctx } from './src/types/ctx';
import { Mp } from './src/types/mp';
import { FunctionInfo } from './src/utils/functionInfo';

declare const mp: Mp;

const config = mp.getServerSettings();

console.log('gamemode.js reloaded');

const quitGamePortal = '42f3f:SweetPie.esp';
const neutralPortal = '42f70:SweetPie.esp';
const redPortal = '42e96:SweetPie.esp';
const bluePortal = '42fc1:SweetPie.esp';

const whiterunSpawnPoint = {
  pos: [22659, -8697, -3594],
  cellOrWorldDesc: '1a26f:Skyrim.esm',
  rot: [0, 0, 268],
};

const whiterunTavernSpawnPoint = {
  pos: [-108, -809, 69.25],
  cellOrWorldDesc: '1605e:Skyrim.esm',
  rot: [0, 0, 176],
};

const hallSpawnPoint = {
  pos: [18511, 10256, 610.6392],
  cellOrWorldDesc: '42b5f:SweetPie.esp',
  rot: [0, 0, 347],
};

const whiterunEscapeDoors = ['1b1f3:Skyrim.esm'];
const whiterunTavernDoorExterior = ['1a6f4:Skyrim.esm'];
const whiterunTavernDoorInterior = ['16072:Skyrim.esm'];

DialogProperty.init();
BrowserProperty.init();
EvalProperty.init();

const enum DialogId {
  NoAction,
  QuitGame,
  JoinDeathMatch,
  LeaveMatch,
}

mp.onActivate = (target: number, caster: number) => {
  const targetDesc = mp.getDescFromId(target);
  if (targetDesc === quitGamePortal) {
    DialogProperty.showMessageBox(caster, DialogId.QuitGame, 'quit game', 'going to leave sweetpie?', ['yes', 'no']);
    return false;
  }
  if (targetDesc === neutralPortal) {
    DialogProperty.showMessageBox(caster, DialogId.JoinDeathMatch, 'deathmatch', 'join deathmatch?', ['yes', 'no']);
    return false;
  }

  const lookupRes = mp.lookupEspmRecordById(target);
  const hasTeleport = lookupRes.record?.fields.findIndex((field) => field.type === 'XTEL') !== -1;
  if (hasTeleport) {
    if (whiterunTavernDoorInterior.indexOf(targetDesc) !== -1) {
      return true;
    } else if (whiterunTavernDoorExterior.indexOf(targetDesc) !== -1) {
      DialogProperty.showMessageBox(
        caster,
        DialogId.NoAction,
        'no way',
        'want to resign? leave the city. you can not hide in tavern',
        ['well']
      );
      return false;
    } else if (whiterunEscapeDoors.indexOf(targetDesc) !== -1) {
      DialogProperty.showMessageBox(caster, DialogId.LeaveMatch, 'run out', 'do you want to run out of town?', [
        'yes',
        'no',
      ]);
      return false;
    } else {
      EvalProperty.eval(caster, (ctx) => {
        ctx.sp.Debug.notification('Interiors are not available during combat');
      });
      return false;
    }
  }

  return true;
};

DialogProperty.setDialogResponseHandler((response) => {
  switch (response.dialogId) {
    case DialogId.QuitGame:
      if (response.buttonIndex === 0) {
        EvalProperty.eval(response.actorId, (ctx: Ctx) => {
          ctx.sp.Game.quitToMainMenu();
          // TODO: close game
        });
      }
      break;
    case DialogId.JoinDeathMatch:
      if (response.buttonIndex === 0) {
        mp.set(response.actorId, 'locationalData', whiterunTavernSpawnPoint);
      }
      break;
    case DialogId.LeaveMatch:
      if (response.buttonIndex === 0) {
        mp.set(response.actorId, 'locationalData', hallSpawnPoint);
      }
      break;
  }
  return true;
});

/*type FrontConstructorValue = {} | null;
type FrontConstructorState = { frontConstructorPrev?: FrontConstructorValue };

const updateOwner = (ctx: Ctx<FrontConstructorState, FrontConstructorValue>) => {
  if (!ctx.value) return;

  // skymp5-client doesn't update references until new value is received
  // so just compare references
  if (ctx.state.frontConstructorPrev === ctx.value) return;
  ctx.state.frontConstructorPrev = ctx.value;

  const stringified = JSON.stringify(ctx.value);
  let src = "";
  src += 'window.tmp = ' + stringified + ';';
  src += 'window.skyrimPlatform.widgets.set(window.tmp);';
  src += "delete window.tmp;";
  ctx.sp.browser.executeJavaScript(src);
};

mp.makeProperty('frontConstructor', {
  isVisibleByOwner: true,
  isVisibleByNeighbors: false,
  updateOwner: new FunctionInfo(updateOwner).getText(),
  updateNeighbor: '',
});

const showDialog = (actorId: number, caption: string, text: string, buttons: string[]): void => {
  const widget = {
    type: 'form',
    id: 1,
    caption: caption,
    elements: [{ type: 'text', text: text, tags: [''] }],
  };
  for (const buttonText of buttons) {
    widget.elements.push({ type: 'button', text: buttonText, tags: ['BUTTON_STYLE_FRAME'] });
  }
  mp.set(actorId, 'frontConstructor', {});
};

const activateQuitGamePortal = (caster: number) => {
  showDialog(caster, "Quit game", "Do you want to quit SweetPie?", ["Yes", "No"])
};

mp.onActivate = (target: number, caster: number) => {
  const targetDesc = mp.getDescFromId(target);
  if (targetDesc === quitGamePortal) {
    activateQuitGamePortal(caster);
    return false;
  }
  return true;
};*/
