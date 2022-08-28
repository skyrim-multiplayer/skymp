import { sprintf } from "sprintf-js";
import { Command } from "./Command";
import { GameModeListenerEx } from "./GameModeListenerEx";
import { PlayerController } from "./PlayerController";

export class GameModeListener implements GameModeListenerEx {
  readonly commands: Command[] = [
    {
      name: 'kick',
      handler: ({ actorId, controller, argsRaw }) => {
        const adminMasterApiIds = [479, 485, 486, 487, 488, 489, 539];
        if (!adminMasterApiIds.includes(controller.getProfileId(actorId))) {
          controller.sendChatMessage(actorId, 'No permission');
          return;
        }
        if (!argsRaw) {
          controller.sendChatMessage(actorId, 'Expected id as an argument');
          return;
        }
        const targetMasterApiId = parseInt(argsRaw);
        for (const targetPlayerActorId of controller.getOnlinePlayers()) {
          if (controller.getProfileId(targetPlayerActorId) === targetMasterApiId) {
            controller.quitGame(targetPlayerActorId);
            controller.sendChatMessage(actorId, `Kicked actor ${targetPlayerActorId.toString(16)}`);
            return;
          }
        }
        controller.sendChatMessage(actorId, 'Not found');
      },
    },
    {
      name: 'kill',
      handler: ({ actorId, controller }) => {
        controller.setPercentages(actorId, { health: 0 });
        controller.sendChatMessage(actorId, 'You killed yourself...');
      }
    },
    {
      name: 'list',
      handler: ({ actorId, controller, argsRaw }) => {
        const data = controller.getOnlinePlayers()
          .map((playerFormId) => ({
            name: controller.getName(playerFormId),
            ids: `${playerFormId.toString(16)}/${controller.getProfileId(playerFormId)}`,
          }))
          .filter(({ name }) => name.toLocaleLowerCase().indexOf(argsRaw?.toLocaleLowerCase() ?? '') !== -1)
          .sort((a, b) => a.name.toLocaleLowerCase().localeCompare(b.name.toLocaleLowerCase()));
        controller.sendChatMessage(actorId, `${data.length} players ${argsRaw ? 'matched' : 'online'}: Server ID / Master API ID - Name`);
        for (const { name, ids } of data) {
          controller.sendChatMessage(actorId, `${ids} - ${name}`);
        }
      },
    },
    {
      name: 'roll',
      handler: ({ senderName, controller, neighbors, inputText }) => {
        const random: string[] = [];
        const [count, _, max]: number[] = inputText.slice(1).split(/(d|к)/g).map(str => parseInt(str));
        const colors: {
          [key: number]: string
        } = {
          2: 'BDBD7D',
          6: 'F78C8C',
          12: '5DAD60',
          20: '7175D6',
          100: '9159B6',
        }
        for (let i = 0; i < count; i++) {
          if (i > 4) break;
          if (max === 2) {
            random.push(Math.floor(Math.random() * (max) + 1) === 2 ? 'успех' : 'неудача');
          } else {
            random.push(`${Math.floor(Math.random() * (max) + 1)}`);
          }
        }
        let message: string;
        if (max === 2) {
          message = `#{${colors[max] ? colors[max] : '9159B6'}}${senderName} подбрасывает монетку #{FFFFFF}- ${random.join(', ')}`;
        } else {
          message = `#{${colors[max] ? colors[max] : '9159B6'}}${senderName} бросает D${max} #{FFFFFF}- ${random.join(', ')}`;
        }
        for (const neighbor of neighbors) {
          controller.sendChatMessage(neighbor, message);
        } 
      },
    },
  ]

  // TODO: Unhardcode this name
  readonly hallSpawnPointName = 'riverwood:spawnPoint';

  constructor(private controller: PlayerController) {

  private getRandomSpawnPoint(points: string[]): string {
    if (points.length) {
      const pIndex = Math.floor(Math.random() * (points.length - 1));
      return points[pIndex];
    }
    return this.hallSpawnPointName;
  }

  onPlayerActivateObject(casterActorId: number, targetObjectDesc: string, targetActorId: number, baseRecType: string): 'continue' | 'blockActivation' {
    if (baseRecType == "DOOR")
    this.controller.sendChatMessage(casterActorId, "A door has been activated.");
    this.controller.sendChatMessage(casterActorId, targetObjectDesc + " : " + targetActorId + " Type:" + baseRecType);
  }

  onPlayerDialogResponse(actorId: number, dialogId: number, buttonIndex: number) {
    // Moving away from confirmation dialogs till some better times...
    // TODO(#835): maybe return the dialog system when bugs are fixed?
  }

  onPlayerChatInput(actorId: number, inputText: string, neighbors: number[], senderName: string) {
    for (const command of this.commands) {
      if (/\/\d+(d|к)\d+/gi.test(inputText) && command.name === 'roll') {
        command.handler({ actorId, controller: this.controller, neighbors, senderName, inputText });
        return;
      }
      if (inputText === '/' + command.name || inputText.startsWith(`/${command.name} `)) {
        command.handler({ actorId, controller: this.controller, neighbors, senderName, inputText, argsRaw: inputText.substring(command.name.length + 2) });
        return;
      }
    }
    for (const neighborActorId of neighbors) {
      this.controller.sendChatMessage(neighborActorId, '' + senderName + ': ' + inputText);
    }
  }

  onPlayerJoin(actorId: number) {
    // Not Implemented
  }

  everySecond() {
    // Not Implemented
  }

  onPlayerDeath(targetActorId: number, killerActorId?: number | undefined) {
    // Not Implemented
  }

  onPlayerLeave(actorId: number) {
    // Not Implemented
  }
}
