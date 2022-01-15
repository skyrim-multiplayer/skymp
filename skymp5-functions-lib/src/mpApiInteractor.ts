import { GameModeListener } from "./logic/GameModeListener";
import { PlayerController } from "./logic/PlayerController";
import { ChatProperty } from "./props/chatProperty";
import { DialogProperty } from "./props/dialogProperty";
import { EvalProperty } from "./props/evalProperty";
import { Ctx } from "./types/ctx";
import { LocationalData, Mp } from "./types/mp";
import { PersistentStorage } from "./utils/persistentStorage";
import { Timer } from "./utils/timer";

declare const mp: Mp;
declare const ctx: Ctx;

const isTeleportDoor = (refrId: number) => {
  const lookupRes = mp.lookupEspmRecordById(refrId);
  if (lookupRes.record) {
    const xtelIndex = lookupRes.record.fields.findIndex((field) => field.type === 'XTEL');
    return xtelIndex !== -1;
  }
  return false;
};

const getName = (actorId: number) => {
  const appearance = mp.get(actorId, 'appearance');
  if (appearance && appearance.name) {
    return `${appearance.name}`;
  }
  return 'Stranger';
};

export class MpApiInteractor {
  static setup(listener: GameModeListener) {
    MpApiInteractor.setupActivateHandler(listener);
    MpApiInteractor.setupChatHandler(listener);
    MpApiInteractor.setupDialogResponseHandler(listener);
    MpApiInteractor.setupTimer(listener);
    MpApiInteractor.setupDeathHandler(listener);
  }

  private static setupActivateHandler(listener: GameModeListener) {
    mp.onActivate = (target: number, caster: number) => {
      const type = mp.get(target, "type");
      if (type !== "MpObjectReference") {
        return true;
      }

      const targetDesc = mp.getDescFromId(target);
      if (!listener.onPlayerActivateObject) {
        return true;
      }

      const isTeleport = isTeleportDoor(target);
      const res = listener.onPlayerActivateObject(caster, targetDesc, isTeleport);
      if (res === 'continue') {
        return true;
      }

      return false;
    };
  }

  private static setupChatHandler(listener: GameModeListener) {
    ChatProperty.setChatInputHandler((input) => {
      // Note that in current implementation we also send chat messages to npcs...
      // TODO: Ignore actorNeighbors that aren't in 'onlinePlayers'
      const actorNeighbors = mp.get(input.actorId, 'actorNeighbors');

      const name = getName(input.actorId);
      if (listener.onPlayerChatInput) {
        listener.onPlayerChatInput(input.actorId, input.inputText, actorNeighbors, name);
      }
    });
  }

  private static setupDialogResponseHandler(listener: GameModeListener) {
    DialogProperty.setDialogResponseHandler((response) => {
      if (listener.onPlayerDialogResponse) {
        listener.onPlayerDialogResponse(response.actorId, response.dialogId, response.buttonIndex);
      }
      return true;
    });
  }

  private static setupTimer(listener: GameModeListener) {
    Timer.everySecond = () => {
      // console.log(PersistentStorage.getSingleton().reloads);

      const onlinePlayers = mp.get(0, 'onlinePlayers');
      const onlinePlayersOld = PersistentStorage.getSingleton().onlinePlayers;

      const joinedPlayers = onlinePlayers.filter((x) => !onlinePlayersOld.includes(x));
      const leftPlayers = onlinePlayersOld.filter((x) => !onlinePlayers.includes(x));

      if (listener.onPlayerJoin) {
        for (const actorId of joinedPlayers) {
          MpApiInteractor.onPlayerJoinHardcoded(actorId);
          listener.onPlayerJoin(actorId);
        }
      }

      if (listener.onPlayerLeave) {
        for (const actorId of leftPlayers) {
          listener.onPlayerLeave(actorId);
        }
      }

      if (listener.everySecond) {
        listener.everySecond();
      }

      if (joinedPlayers.length > 0 || leftPlayers.length > 0) {
        PersistentStorage.getSingleton().onlinePlayers = onlinePlayers;
      }
    };
  }

  private static setupDeathHandler(listener: GameModeListener) {
    mp.onDeath = (target: number, killer: number) => {
      if (listener.onPlayerDeath) {
        if (killer === 0) {
          listener.onPlayerDeath(target, undefined);
        }
        else {
          listener.onPlayerDeath(target, killer);
        }
      }
    };
  }

  private static onPlayerJoinHardcoded(actorId: number) {
    ChatProperty.showChat(actorId, true);
  }

  static makeController(pointsByName: Map<string, LocationalData>): PlayerController {
    return {
      setSpawnPoint(player: number, pointName: string) {
        const point = pointsByName.get(pointName);
        if (point) {
          mp.set(player, 'spawnPoint', point);
        }
        else {
          console.log(`setSpawnPoint: point not found - ${pointName}`);
        }
      },
      teleport(player: number, pointName: string): void {
        const point = pointsByName.get(pointName);
        if (point) {
          mp.set(player, 'locationalData', point);
        }
        else {
          console.log(`teleport: point not found - ${pointName}`);
        }
      },
      showMessageBox(actorId: number, dialogId: number, caption: string, text: string, buttons: string[]): void {
        DialogProperty.showMessageBox(actorId, dialogId, caption.toLowerCase(), text.toLowerCase(), buttons.map(x => x.toLowerCase()));
      },
      sendChatMessage(actorId: number, text: string): void {
        ChatProperty.sendChatMessage(actorId, text);
      },
      quitGame(actorId: number): void {
        EvalProperty.eval(actorId, () => {
          ctx.sp.Game.quitToMainMenu();
          // TODO: close game
        });
      },
      getName(actorId: number): string {
        return getName(actorId);
      }
    }
  }
}
