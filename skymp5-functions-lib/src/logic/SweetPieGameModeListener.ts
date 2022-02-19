import { sprintf } from "../lib/sprintf-js";
import { Command } from "./Command";
import { GameModeListener } from "./GameModeListener";
import { PlayerController } from "./PlayerController";
import { SweetPieMap } from "./SweetPieMap";
import { forceLeaveRound, getPlayerCurrentRound, getAvailableRound, forceJoinRound, determineDeathMatchWinners, SweetPieRound } from "./SweetPieRound";

export class SweetPieGameModeListener implements GameModeListener {
  readonly coinFormId = 0xf;
  readonly applePieFormId = 0x00064B43;
  readonly goldOreFormId = 0x0005ACDE;
  readonly silverOreFormId = 0x0005ACDF;

  readonly quitGamePortal = '42f3f:SweetPie.esp';
  readonly neutralPortal = '42f70:SweetPie.esp';
  readonly redPortal = '42e96:SweetPie.esp';
  readonly bluePortal = '42fc1:SweetPie.esp';

  readonly noEnterSafePlaceMessage: [string] = ["Странно, но вы не можете открыть эту дверь. Похоже, можно только сбежать через ворота..."];
  readonly interiorsBlockedMessage: [string] = ["Похоже, эта дверь не откроется. Придется драться..."];
  readonly warmupFinishedMessage: [string] = ["Разминка окончена. У вас есть %d секунд, чтобы убить друг друга!"];
  readonly startingRoundInMessage: [string] = ["Раунд начнется через %d секунд"];
  readonly remainingFightTimeMessage: [string] = ["В бой! У вас %d секунд"];
  readonly determineWinnerMessage: [string] = ["%s побеждает с %d очками! Спасибо за игру!"];
  readonly noWinnerMessage: [string] = ["Победителя нет. Спасибо за игру!"];
  readonly multipleWinnersMessage: [string] = ["У нас несколько победителей!"];
  readonly deathMessage: [string] = ["%s был убит %s. У %s теперь %d очков (у лучшего игрока %d)"];
  readonly restoreMessage: [string] = ["Полное восстановление"];
  readonly restoreDeniedMessage: [string] = ["Подождите еще %d секунд перед повторным восстановлением"];

  readonly cantStartMessage: [string] = ["Игроков слишком мало, разминка начнется, когда будет еще %s человек"];

  readonly comingSoonPortalName = 'Скоро...';
  readonly quitGamePortalName = 'Выйти из игры и вернуться на рабочий стол';
  readonly returnToHallPortalName = 'Вернуться в чертоги';
  readonly neutralPortalNameTpl = 'Войти в десматч\nИгроков сейчас: %d (мин %d для старта игры)\n%s';

  readonly roundStateToHumanReadable: Record<SweetPieRound['state'], string> = {
    'wait': 'Ожидание игроков...',
    'warmup': 'Разминка',
    'running': 'Игра идет, подождите',
    'finished': 'Игра идет, подождите',
  };
  readonly commands: Command[] = [
    {
      name: 'kill',
      handler: (actorId: number, controller: PlayerController) => {
        controller.setPercentages(actorId, { health: 0 });
        controller.sendChatMessage(actorId, 'Вы убили себя...');
      }
    }, 
  ]

  warmupTimerMaximum = 60;
  runningTimerMaximum = 300;
  roundEndTimerMaximum = 5;

  // TODO: Unhardcode this name
  readonly hallSpawnPointName = 'hall:spawnPoint';

  constructor(private controller: PlayerController, private maps: SweetPieMap[] = [], private minimumPlayersToStart: number = 5) {
    this.rounds = this.controller.getRoundsArray();
    if (this.rounds.length === 0) {
      maps.forEach(map => this.rounds.push({ state: 'wait', map: map, hallPointName: this.hallSpawnPointName, secondsPassed: 0 }));
    } else {
      if (maps.length > this.rounds.length) {
        for (var i = this.rounds.length; i < maps.length; i++) {
          this.rounds.push({ state: 'wait', hallPointName: this.hallSpawnPointName, secondsPassed: 0 });
        }
      } else if (maps.length < this.rounds.length) {
        const toDelete = [];
        for (var i = 0; i < this.rounds.length; i++) {
          if (this.rounds[i].players?.size) {
            continue;
          }
          toDelete.push(i);
        }
        toDelete.forEach(index => this.rounds.splice(index, 1));
      }
      for (var i = 0; i < this.rounds.length; i++) {
        if (this.rounds[i].players?.size) {
          continue;
        }
        this.rounds[i].map = maps[i];
      }
    }
    this.controller.setRoundsArray(this.rounds);
    this.controller.updateCustomName(this.quitGamePortal, this.quitGamePortalName);
    this.controller.updateCustomName(this.redPortal, this.comingSoonPortalName);
    this.controller.updateCustomName(this.bluePortal, this.comingSoonPortalName);
    // FIXME: does not apply in-game
    this.rounds.forEach((round) => round.map?.leaveRoundDoors?.forEach(
      (doorDesc) => this.controller.updateCustomName(doorDesc, this.returnToHallPortalName)
    ));
  }

  private resetRound(roundIndex: number) {
    const players = this.rounds[roundIndex].players;
    if (players) {
      for (const [player] of players) {
        forceLeaveRound(this.controller, this.rounds, player);
      }
    }
    if (this.maps.length == this.rounds.length || roundIndex < this.maps.length) {
      this.rounds[roundIndex] = { state: 'wait', map: this.maps[roundIndex], hallPointName: this.hallSpawnPointName, secondsPassed: 0 }
    } else if (roundIndex >= this.maps.length) {
      this.rounds.splice(roundIndex, 1);
    }
    this.controller.setRoundsArray(this.rounds);
  }

  private getRandomSpawnPoint(points: string[]): string {
    if (points.length) {
      const pIndex = Math.floor(Math.random() * (points.length - 1));
      return points[pIndex];
    }
    return this.hallSpawnPointName;
  }

  private updatePortalsByAvailableRounds(): void {
    const round = getAvailableRound(this.rounds) || this.rounds.find((x) => x.map?.enabled && x.state === 'running');
    if (round) {
      const playersCount = round.players?.size || 0;
      this.controller.updateCustomName(
        this.neutralPortal,
        sprintf(this.neutralPortalNameTpl, playersCount, this.minimumPlayersToStart, this.roundStateToHumanReadable[round.state]),
      );
    }
  }

  getRounds() {
    return this.rounds;
  }

  onPlayerActivateObject(casterActorId: number, targetObjectDesc: string, targetActorId: number): 'continue' | 'blockActivation' {
    if (targetObjectDesc === this.quitGamePortal) {
      this.controller.quitGame(casterActorId);
      return 'continue';
    } else if (targetObjectDesc === this.neutralPortal) {
      const round = getAvailableRound(this.rounds, casterActorId);
      if (!round || !round.map) {
        // TODO: Handle unavailability to find a round
      } else {
        forceJoinRound(this.controller, this.rounds, round, casterActorId);
        this.controller.setRoundsArray(this.rounds);
      }
      return 'continue';
    } else {
      let round = getPlayerCurrentRound(this.rounds, casterActorId);
      if (!round) {
        // We aren't aware of any round this player might be in.
        // However, if they somehow got into the battlefield, we should let them return to lobby...
        round = this.rounds.find((x) => x.map?.leaveRoundDoors?.includes(targetObjectDesc));
        if (round?.hallPointName) {
          this.controller.setSpawnPoint(casterActorId, round.hallPointName);
          this.controller.teleport(casterActorId, round.hallPointName);
          return 'continue';
        }
        const rx = RegExp("^sweet.*(Eat|Drink|Soup)", "i"); // TODO: Make this configurable
        if (rx.test(this.controller.getScriptName(targetActorId))) {
          const percentages = this.controller.getPercentages(casterActorId);
          this.controller.setPercentages(
            casterActorId, {
              health: percentages.health! + .5,
              magicka: percentages.magicka! + .5,
              stamina: percentages.stamina! + .5
          });
          return 'continue';
        }
      }
      if (round && round.map) {
        if (round.map.safePlaceEnterDoors?.includes(targetObjectDesc)) {
          if (round.state !== 'wait') {
            this.controller.sendChatMessage(casterActorId, ...this.noEnterSafePlaceMessage);
            return 'blockActivation';
          }
          return 'continue';
        }
        if (round.map.leaveRoundDoors?.includes(targetObjectDesc)) {
          this.onPlayerLeave(casterActorId);
          return 'continue';
        }
        if (round.map.safePlaceLeaveDoors?.includes(targetObjectDesc)) {
          return 'continue';
        }
        if (round.map.playerRestoreActivators?.includes(targetObjectDesc)) {
          const now = Date.now();
          const elapsed = now - (round.players?.get(casterActorId)?.restored || now);
          const waitTime = round.map.playerRestoreWaitTime || 30000;
          if (elapsed >= 0) {
            round.players!.get(casterActorId)!.restored = now + waitTime;
            this.controller.setPercentages(casterActorId, {});
            this.controller.sendChatMessage(casterActorId, ...this.restoreMessage);
            return 'continue';
          }
          this.controller.sendChatMessage(casterActorId, sprintf(this.restoreDeniedMessage[0], -elapsed / 1000));
          return 'continue';
          }
        if (this.controller.isTeleportActivator(targetActorId)) {
          this.controller.sendChatMessage(casterActorId, ...this.interiorsBlockedMessage);
          return 'blockActivation';
        }
      }
    }
    return 'continue';
  }

  onPlayerDialogResponse(actorId: number, dialogId: number, buttonIndex: number) {
    // Moving away from confirmation dialogs till some better times...
    // TODO(#835): maybe return the dialog system when bugs are fixed?
  }

  onPlayerChatInput(actorId: number, inputText: string, neighbors: number[], senderName: string) {
    for (const command of this.commands) {
      if (inputText === '/' + command.name) {
        command.handler(actorId, this.controller);
        return;
      }
    }
    for (const neighborActorId of neighbors) {
      this.controller.sendChatMessage(neighborActorId, '' + senderName + ': ' + inputText);
    }
  }

  onPlayerJoin(actorId: number) {
    this.controller.setSpawnPoint(actorId, this.hallSpawnPointName);
  }

  everySecond() {
    this.updatePortalsByAvailableRounds();
    for (const round of this.rounds) {
      if (round.players && round.players.size) {
        round.secondsPassed = (round.secondsPassed ?? -1) + 1;
        if (round.state === 'warmup' || round.state === 'wait') {
          const onlinePlayers: number[] = this.controller.getOnlinePlayers();
          const playersToRemove: number[] = [];
          for (const player of round.players.keys()) {
            if (onlinePlayers.indexOf(player) < 0) {
              playersToRemove.push(player);
            }
          }
          for (const player of playersToRemove) {
            forceLeaveRound(this.controller, this.rounds, player);
          }
          if (0 >= round.players.size) {
            this.resetRound(this.rounds.indexOf(round));
            continue;
          }
          if (round.players.size < this.minimumPlayersToStart) {
            if (round.state === 'warmup') {
              round.state = 'wait';
              round.secondsPassed = 0;
            }
            if (this.sendMessageNeeded(this.warmupTimerMaximum - round.secondsPassed)) {
              this.sendRoundChatMessage(round, sprintf(this.cantStartMessage[0], this.minimumPlayersToStart - round.players.size));
              round.secondsPassed = 0;
            }
            continue;
          } else if (round.state === 'wait') {
            round.state = 'warmup';
            round.secondsPassed = 0;
          }
          const secondsRemaining = this.warmupTimerMaximum - round.secondsPassed;
          if (secondsRemaining > 0 && this.sendMessageNeeded(secondsRemaining)) {
            this.sendRoundChatMessage(round, sprintf(this.startingRoundInMessage[0], secondsRemaining));
          }
          if (round.secondsPassed > this.warmupTimerMaximum) {
            round.secondsPassed = 0;
            round.state = 'running';
            this.sendRoundChatMessage(round, sprintf(this.warmupFinishedMessage[0], this.runningTimerMaximum));
            for (const [player] of round.players) {
              if (round.map && round.map.spawnPointNames) {
                const pName = this.getRandomSpawnPoint(round.map.spawnPointNames);
                this.controller.setSpawnPoint(player, pName);
                this.controller.teleport(player, pName);
                this.controller.setPercentages(player, {});
              }
            }
          }
        }
        else if (round.state === 'running') {
          const secondsRemaining = this.runningTimerMaximum - round.secondsPassed;
          if (secondsRemaining > 0 && this.sendMessageNeeded(secondsRemaining)) {
            this.sendRoundChatMessage(round, sprintf(this.remainingFightTimeMessage[0], secondsRemaining));
          }
          if (round.secondsPassed > this.runningTimerMaximum) {
            if (round.players) {
              for (const [player,] of round.players) {
                const numGamesBefore = this.controller.incrementCounter(player, 'finishedDeathmatches', 1);
                const rewardFormId = (numGamesBefore < 3 ? this.goldOreFormId : this.silverOreFormId);
                this.controller.addItem(player, rewardFormId, 1);
              }
            }
            const winners = determineDeathMatchWinners(round);
            if (winners.length === 0) {
              this.sendRoundChatMessage(round, ...this.noWinnerMessage);
            } else {
              if (winners.length > 1) {
                this.sendRoundChatMessage(round, ...this.multipleWinnersMessage);
              }
              for (const winner of winners) {
                this.controller.addItem(winner, this.coinFormId, 15);
                const winnerScore = round.players.get(winner)?.kills;
                this.sendRoundChatMessage(round, sprintf(this.determineWinnerMessage[0], this.controller.getName(winner), winnerScore));
              }
            }
            round.secondsPassed = 0;
            round.state = 'finished';
          }
        } else if (round.state === 'finished') {
          if (round.secondsPassed > this.roundEndTimerMaximum) {
            this.resetRound(this.rounds.indexOf(round));
          }
        }
      }
    }
    this.controller.setRoundsArray(this.rounds);
  }

  onPlayerDeath(targetActorId: number, killerActorId?: number | undefined) {
    const round = getPlayerCurrentRound(this.rounds, targetActorId);
    if (killerActorId) {
      const round2 = getPlayerCurrentRound(this.rounds, killerActorId);
      if (round === round2 && round && round.players && round.state === 'running') {
        this.controller.addItem(killerActorId, this.coinFormId, 1);
        const killerState = round.players.get(killerActorId);
        if (killerState) {
          killerState.kills = (killerState.kills || 0) + 1;
        }
        const killerScore = round.players.get(killerActorId)?.kills;
        const winnerScore = Math.max(...(determineDeathMatchWinners(round).map(x => round.players?.get(x)?.kills) as number[]));
        this.sendRoundChatMessage(round, sprintf(this.deathMessage[0], this.controller.getName(targetActorId), this.controller.getName(killerActorId), this.controller.getName(killerActorId), killerScore, winnerScore));
      }
    }
    if (round?.state === 'running' && round.map && round.map.spawnPointNames) {
      this.controller.setSpawnPoint(targetActorId, this.getRandomSpawnPoint(round.map.spawnPointNames));
    }
    this.controller.setRoundsArray(this.rounds);
  }

  onPlayerLeave(actorId: number) {
    const round = getPlayerCurrentRound(this.rounds, actorId);
    forceLeaveRound(this.controller, this.rounds, actorId);
    if (round && round.players?.size === 0) {
      const roundIndex = this.rounds.indexOf(round);
      this.resetRound(roundIndex);
    }
    this.controller.setRoundsArray(this.rounds);
  }

  private sendRoundChatMessage(round: SweetPieRound, msg: string) {
    for (const [player] of (round.players || new Map)) {
      this.controller.sendChatMessage(player, msg);
    }
  }

  private sendMessageNeeded(secondsRemaining: number) {
    return secondsRemaining <= 10 || secondsRemaining % 10 === 0;
  };

  private rounds: SweetPieRound[];
}
