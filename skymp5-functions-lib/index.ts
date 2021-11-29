import { BrowserProperty } from './src/props/browserProperty';
import { ChatProperty } from './src/props/chatProperty';
import { DialogProperty } from './src/props/dialogProperty';
import { EvalProperty } from './src/props/evalProperty';
import { Ctx } from './src/types/ctx';
import { LocationalData, Mp } from './src/types/mp';
import { FunctionInfo } from './src/utils/functionInfo';
import { PersistentStorage } from './src/utils/persistentStorage';
import { Timer } from './src/utils/timer';

DialogProperty.init();
BrowserProperty.init();
EvalProperty.init();
ChatProperty.init();

Timer.init();

declare const mp: Mp;

const config = mp.getServerSettings();

console.log('gamemode.js reloaded');

const quitGamePortal = '42f3f:SweetPie.esp';
const neutralPortal = '42f70:SweetPie.esp';
const redPortal = '42e96:SweetPie.esp';
const bluePortal = '42fc1:SweetPie.esp';

const hallSpawnPoint = {
  pos: [18511, 10256, 610.6392],
  cellOrWorldDesc: '42b5f:SweetPie.esp',
  rot: [0, 0, 347],
};

type SpawnPoint = LocationalData;

interface SweetPieMap {
  // TODO: custom blue/red spawnpoints
  mainSpawnPoint: SpawnPoint;

  safePlace: SpawnPoint;
  safePlaceGoOutDoors: string[];
  safePlaceComeInDoors: string[];

  leaveMapDoors: string[];
}

const warmupTimerMaximum = 60;
const runningTimerMaximum = 300;

class SweetPieRound {
  constructor(public readonly map: SweetPieMap) {}

  warmupTimer = warmupTimerMaximum;
  runningTimer = runningTimerMaximum;
  maxPlayers = 20;
  players = new Set<number>();
  state: 'running' | 'warmup' = 'warmup';
  score = new Map<number, number>();
}

const whiterun: SweetPieMap = {
  mainSpawnPoint: {
    pos: [22659, -8697, -3594],
    cellOrWorldDesc: '1a26f:Skyrim.esm',
    rot: [0, 0, 268],
  },
  safePlace: {
    pos: [-108, -809, 69.25],
    cellOrWorldDesc: '1605e:Skyrim.esm',
    rot: [0, 0, 176],
  },
  safePlaceGoOutDoors: ['16072:Skyrim.esm'],
  safePlaceComeInDoors: ['1a6f4:Skyrim.esm'],
  leaveMapDoors: ['1b1f3:Skyrim.esm'],
};

const rounds = new Array<SweetPieRound>();
rounds.push(new SweetPieRound(whiterun));

const findRoundForPlayer = (player: number): SweetPieRound | undefined => {
  return rounds.find((x) => x.state !== 'running');
};

const getPlayerCurrentRound = (player: number): SweetPieRound | undefined => {
  return rounds.find((x) => x.players.has(player));
};

const joinRound = (round: SweetPieRound, player: number) => {
  mp.set(player, 'spawnPoint', round.map.safePlace);
  mp.set(player, 'locationalData', round.map.safePlace);
  round.players.add(player);
};

const leaveRound = (round: SweetPieRound | undefined, player: number) => {
  mp.set(player, 'spawnPoint', hallSpawnPoint);
  mp.set(player, 'locationalData', hallSpawnPoint);
  round?.players.delete(player);
};

const getWinner = (round: SweetPieRound) => {
  let bestActorId = 0;
  let bestScore = -1;
  for (const actorId of round.players) {
    const score = round.score.get(actorId) || 0;
    if (score > bestScore) {
      bestScore = score;
      bestActorId = actorId;
    }
  }
  return bestActorId;
};

const sendMessageNeeded = (timerSeconds: number) => {
  return timerSeconds <= 10 || timerSeconds % 10 === 0;
};

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
    const round = getPlayerCurrentRound(caster);
    if (round && round.map.safePlaceComeInDoors.indexOf(targetDesc) !== -1) {
      DialogProperty.showMessageBox(
        caster,
        DialogId.NoAction,
        'no way',
        'want to resign? leave the city. you can not hide in tavern',
        ['well']
      );
      return false;
    } else if (!round || round.map.leaveMapDoors.indexOf(targetDesc) !== -1) {
      DialogProperty.showMessageBox(caster, DialogId.LeaveMatch, 'run out', 'do you want to run out of town?', [
        'yes',
        'no',
      ]);
      return false;
    } else if (round.map.safePlaceGoOutDoors.indexOf(targetDesc) !== -1) {
      return true;
    } else {
      ChatProperty.sendChatMessage(caster, 'Interiors are not available during combat');
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
        const round = findRoundForPlayer(response.actorId);
        if (round) {
          joinRound(round, response.actorId);
        } else {
          DialogProperty.showMessageBox(
            response.actorId,
            DialogId.NoAction,
            'no free maps',
            'sorry, there are no free slots for new players',
            ['well']
          );
        }
      }
      break;
    case DialogId.LeaveMatch:
      if (response.buttonIndex === 0) {
        const round = findRoundForPlayer(response.actorId);
        leaveRound(round, response.actorId);
      }
      break;
  }
  return true;
});

const getName = (actorId: number) => {
  const appearance = mp.get(actorId, 'appearance');
  if (appearance && appearance.name) {
    return `${appearance.name}`;
  }
  return 'Stranger';
};

ChatProperty.setChatInputHandler((input) => {
  // Note that in current implementation we also send chat messages to npcs...
  const actorNeighbors = mp.get(input.actorId, 'actorNeighbors');

  const name = getName(input.actorId);

  for (const neighborActorId of actorNeighbors) {
    ChatProperty.sendChatMessage(neighborActorId, '#{a8adad}' + name + '#{ffffff}: ' + input.inputText);
  }
});

const onJoin = (actorId: number) => {
  ChatProperty.showChat(actorId, true);
  mp.set(actorId, 'spawnPoint', hallSpawnPoint);
};

const onLeave = (actorId: number) => {};

const everySecond = () => {
  for (const round of rounds) {
    if (round.state === 'warmup') {
      if (round.players.size > 0) {
        if (round.warmupTimer > 0) {
          round.warmupTimer--;
          if (sendMessageNeeded(round.warmupTimer)) {
            round.players.forEach((actorId) =>
              ChatProperty.sendChatMessage(actorId, `Starting round in ${round.warmupTimer}`)
            );
          }
        } else {
          round.players.forEach((actorId) => {
            mp.set(actorId, 'spawnPoint', round.map.mainSpawnPoint);
            ChatProperty.sendChatMessage(
              actorId,
              `Warmup finished! Go! You have ${round.runningTimer} seconds to kill each other!`
            );
          });
          round.players.forEach((actorId) => mp.set(actorId, 'locationalData', round.map.mainSpawnPoint));
          round.state = 'running';
          round.score = new Map();
          round.warmupTimer = warmupTimerMaximum;
        }
      }
    }
    if (round.state === 'running') {
      if (round.runningTimer > 0) {
        round.runningTimer--;
        if (sendMessageNeeded(round.runningTimer)) {
          round.players.forEach((actorId) => {
            ChatProperty.sendChatMessage(actorId, `Fight! You have ${round.runningTimer} seconds`);
          });
        }
      } else {
        const winnerActorId = getWinner(round);
        const winner = winnerActorId ? getName(winnerActorId) : 'No one';
        const score = round.score.get(winnerActorId);
        round.players.forEach((actorId) =>
          ChatProperty.sendChatMessage(actorId, `${winner} wins with ${score} points! Thanks for playing`)
        );
        round.players.forEach((actorId) => leaveRound(round, actorId));
        round.state = 'warmup';
        round.runningTimer = runningTimerMaximum;
      }
    }
  }
};

mp.onDeath = (target: number, killer: number) => {
  const round = rounds.find((x) => x.players.has(target));
  if (!round) return;

  round.players.forEach((actorId) => {
    ChatProperty.sendChatMessage(actorId, `#{a8adad}${getName(target)} was slain by ${getName(killer)}`);
    const score = round.score.get(killer);
    const newScore = score ? score + 1 : 1;
    round.score.set(killer, newScore);
    ChatProperty.sendChatMessage(actorId, `#{a8adad}${getName(killer)} now has ${newScore} points`);
  });
};

Timer.everySecond = () => {
  // console.log(PersistentStorage.getSingleton().reloads);

  const onlinePlayers = mp.get(0, 'onlinePlayers');
  const onlinePlayersOld = PersistentStorage.getSingleton().onlinePlayers;

  const joinedPlayers = onlinePlayers.filter((x) => !onlinePlayersOld.includes(x));
  const leftPlayers = onlinePlayersOld.filter((x) => !onlinePlayers.includes(x));

  for (const actorId of joinedPlayers) {
    onJoin(actorId);
  }

  for (const actorId of leftPlayers) {
    onLeave(actorId);
  }

  everySecond();

  if (joinedPlayers.length > 0 || leftPlayers.length > 0) {
    PersistentStorage.getSingleton().onlinePlayers = onlinePlayers;
  }
};
