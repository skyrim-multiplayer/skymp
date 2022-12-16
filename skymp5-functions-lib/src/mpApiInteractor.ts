import { GameModeListener } from "./logic/GameModeListener";
import { Counter, Percentages, PlayerController } from "./logic/PlayerController";
import { SweetPieRound } from "./logic/SweetPieRound";
import { ChatMessage, ChatNeighbor, ChatProperty } from "./props/chatProperty";
import { CounterProperty } from "./props/counterProperty";
import { DialogProperty } from "./props/dialogProperty";
import { EvalProperty } from "./props/evalProperty";
import { Ctx } from "./types/ctx";
import { LocationalData, Mp, PapyrusObject } from "./types/mp";
import { ChatSettings } from "./types/settings";
import { PersistentStorage } from "./utils/persistentStorage";
import { Timer } from "./utils/timer";

declare const mp: Mp;
declare const ctx: Ctx;
declare const nameUpdatesClientSide: [number, string][];

const scriptName = (refrId: number) => {
  const lookupRes = mp.lookupEspmRecordById(refrId);
  if (lookupRes.record) {
    const vmadIndex = lookupRes.record.fields.findIndex((field) => field.type === 'VMAD');
    if (vmadIndex >= 0) {
      const vmadData = lookupRes.record.fields[vmadIndex].data;
      const strLength = vmadData[6] + (vmadData[7] << 8);
      var strData: string = '';
      for (var i = 0; i < strLength; i++) {
        strData += String.fromCharCode(vmadData[8+i]).valueOf();
      }
      return strData;
    }
  }
  return '';
}

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

const sqr = (x: number) => x * x;

const getActorDistanceSquared = (actorId1: number, actorId2: number) => {
  const pos1 = mp.get(actorId1, 'pos');
  const pos2 = mp.get(actorId2, 'pos');
  const delta = [pos1[0] - pos2[0], pos1[1] - pos2[1], pos1[2] - pos2[2]];
  return sqr(delta[0]) + sqr(delta[1]) + sqr(delta[2]);
};

export class MpApiInteractor {
  private static serverSettings = mp.getServerSettings();
  private static customNames = new Map<number, string>();

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

      const res = listener.onPlayerActivateObject(caster, targetDesc, target);
      if (res === 'continue') {
        return true;
      }

      return false;
    };
  }

  private static setupChatHandler(listener: GameModeListener) {
    ChatProperty.setChatInputHandler((input) => {
      const chatSettings = this.serverSettings.sweetpieChatSettings as ChatSettings ?? {};
      const hearingRadius = chatSettings.hearingRadiusNormal !== undefined ?
                            sqr(chatSettings.hearingRadiusNormal) :
                            sqr(2000)
      const onlinePlayers = mp.get(0, 'onlinePlayers');
      const actorNeighbors =
        mp.get(input.actorId, 'actorNeighbors')
        .filter((actorId) => onlinePlayers.indexOf(actorId) !== -1)
        .reduce<ChatNeighbor[]>((filtered, actorId) => {
          const distance = getActorDistanceSquared(input.actorId, actorId)
          if (distance < hearingRadius) {
            filtered.push({
              actorId,
              opacity: Number(((hearingRadius - distance) / hearingRadius).toFixed(3))
            })
          }
          return filtered
        }, [])
      const name = getName(input.actorId);
      if (listener.onPlayerChatInput) {
        console.log(`chat: ${JSON.stringify(name)} (${input.actorId.toString(16)}): ${JSON.stringify(input.inputText)}`);
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

      for (const actorId of leftPlayers) {
        mp.set(actorId, 'eval', { commands: [], nextId: 0 });
      }

      for (const actorId of onlinePlayers) {
        const nameUpdates: [number, string][] = [];
        for (const formId of mp.get(actorId, 'neighbors')) {
          const name = MpApiInteractor.customNames.get(formId);
          if (name !== undefined) {
            nameUpdates.push([formId, name]);
          }
        }
        if (!nameUpdates.length) {
          continue;
        }
        EvalProperty.eval(actorId, () => {
          for (const [formId, name] of nameUpdatesClientSide) {
            const refr = ctx.sp.ObjectReference.from(ctx.sp.Game.getFormEx(formId));
            const ret = refr?.setDisplayName(name, true);
            if (!ret) {
              ctx.sp.printConsole('setDisplayName failed:', name, refr, ret);
            }
          }
        }, { nameUpdatesClientSide: nameUpdates });
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
        } else {
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
        } else {
          console.log(`setSpawnPoint: point not found - ${pointName}`);
        }
      },
      teleport(player: number, pointName: string): void {
        const point = pointsByName.get(pointName);
        if (point) {
          mp.set(player, 'locationalData', point);
        } else {
          console.log(`teleport: point not found - ${pointName}`);
        }
      },
      showMessageBox(actorId: number, dialogId: number, caption: string, text: string, buttons: string[]): void {
        DialogProperty.showMessageBox(actorId, dialogId, caption.toLowerCase(), text.toLowerCase(), buttons.map(x => x.toLowerCase()));
      },
      sendChatMessage(actorId: number, message: ChatMessage): void {
        ChatProperty.sendChatMessage(actorId, message);
      },
      quitGame(actorId: number): void {
        EvalProperty.eval(actorId, () => {
          ctx.sp.win32.exitProcess();
        });
      },
      getName(actorId: number): string {
        return getName(actorId);
      },
      getProfileId(playerActorId: number): number {
        return mp.get(playerActorId, 'profileId');
      },
      addItem(actorId: number, itemId: number, count: number): void {
        mp.callPapyrusFunction(
          'method', 'ObjectReference', 'AddItem',
          { type: 'form', desc: mp.getDescFromId(actorId) },
          [{ type: 'espm', desc: mp.getDescFromId(itemId) }, count, /*silent*/false]
        );
      },
      getRoundsArray(): SweetPieRound[] {
        return PersistentStorage.getSingleton().rounds;
      },
      setRoundsArray(rounds: SweetPieRound[]): void {
        PersistentStorage.getSingleton().rounds = rounds;
      },
      getOnlinePlayers(): number[] {
        return PersistentStorage.getSingleton().onlinePlayers;
      },
      setPercentages(actorId: number, percentages: Percentages): void {
        mp.set(
          actorId,
          'percentages',
          {
            health: percentages.health ?? 1.0,
            magicka: percentages.magicka ?? 1.0,
            stamina: percentages.stamina ?? 1.0
          });
      },
      getPercentages(actorId: number): Percentages {
        return mp.get(actorId, 'percentages');
      },
      getScriptName(refrId: number): string {
        return scriptName(refrId);
      },
      isTeleportActivator(refrId: number): boolean {
        return isTeleportDoor(refrId);
      },
      updateCustomName(formDesc: string, name: string): void {
        MpApiInteractor.customNames.set(mp.getIdFromDesc(formDesc), name);
      },
      incrementCounter(actorId: number, counter: Counter, by?: number): number {
        const current = CounterProperty.get(actorId, counter);
        CounterProperty.set(actorId, counter, current + (by ?? 0));
        return current;
      },
    }
  }
}
