import { sprintf } from "../lib/sprintf-js";
import { GameModeListener } from "./GameModeListener";
import { PlayerController } from "./PlayerController";
import { SweetPieMap } from "./SweetPieMap";
import { forceLeaveRound, getPlayerCurrentRound, getAvailableRound, forceJoinRound, determineDeathMatchWinners, SweetPieRound } from "./SweetPieRound";

export class SweetPieGameModeListener implements GameModeListener {
  readonly quitGamePortal = '42f3f:SweetPie.esp';
  readonly neutralPortal = '42f70:SweetPie.esp';
  readonly redPortal = '42e96:SweetPie.esp';
  readonly bluePortal = '42fc1:SweetPie.esp';

  readonly quitDialog: [number, string, string, string[]] = [1001, "Quit game", "Do you want to quit SweetPie?", ["Yes", "No"]];
  readonly joinDeathMatchDialog: [number, string, string, string[]] = [1002, "Join DeathMatch", "Do you want to join DeathMatch?", ["Yes", "No"]];
  readonly noEnterSafePlaceDialog: [number, string, string, string[]] = [1003, "No enter", "You cannot go back to safety! Running out of the map is the only option", ["Well"]];
  readonly leaveRoundConfirmationDialog: [number, string, string, string[]] = [1004, "Leave round", "Do you want to run out?", ["Yes", "No"]];

  readonly interiorsBlockedMessage: [string] = ["Interiors are not available during round"];
  readonly warmupFinishedMessage: [string] = ["Warmup finished! Go! You have %d seconds to kill each other!"];
  readonly startingRoundInMessage: [string] = ["Starting round in %d seconds"];
  readonly remainingFightTimeMessage: [string] = ["Fight! You have %d seconds"];
  readonly determineWinnerMessage: [string] = ["%s wins with %d points! Thanks for playing"];
  readonly noWinnerMessage: [string] = ["There is no winner! Thanks for playing"];
  readonly multipleWinnersMessage: [string] = ["We have multiple winners!"];
  readonly deathMessage: [string] = ["%s was slain by %s. %s now has %d points (the best is %d)"];

  warmupTimerMaximum = 60;
  runningTimerMaximum = 300;

  // TODO: Unhardcode this name
  readonly hallSpawnPointName = 'hall:spawnPoint';

  constructor(private controller: PlayerController, private maps: SweetPieMap[] = []) {
    this.rounds = [];
    maps.forEach(map => this.rounds.push({ state: 'warmup', map: map }));
    this.rounds.forEach((round, index) => this.resetRound(index));
  }

  private resetRound(roundIndex: number) {
    const players = this.rounds[roundIndex].players;
    if (players) {
      for (const [player] of players) {
        forceLeaveRound(this.controller, this.rounds, player);
      }
    }
    this.rounds[roundIndex] = { state: 'warmup', map: this.rounds[roundIndex].map, hallPointName: this.hallSpawnPointName, secondsPassed: 0 }
  }

  getRounds() {
    return this.rounds;
  }

  onPlayerActivateObject(casterActorId: number, targetObjectDesc: string, isTeleportDoor: boolean): 'continue' | 'blockActivation' {
    if (targetObjectDesc === this.quitGamePortal) {
      this.controller.showMessageBox(casterActorId, ...this.quitDialog);
      return 'blockActivation';
    }
    else if (targetObjectDesc === this.neutralPortal) {
      this.controller.showMessageBox(casterActorId, ...this.joinDeathMatchDialog);
      return 'blockActivation';
    }
    else {
      const round = getPlayerCurrentRound(this.rounds, casterActorId);
      if (round && round.map) {
        if (round.map.safePlaceEnterDoors?.includes(targetObjectDesc)) {
          this.controller.showMessageBox(casterActorId, ...this.noEnterSafePlaceDialog);
          return 'blockActivation';
        }
        if (round.map.leaveRoundDoors?.includes(targetObjectDesc)) {
          this.controller.showMessageBox(casterActorId, ...this.leaveRoundConfirmationDialog);
          return 'blockActivation';
        }
        if (round.map.safePlaceLeaveDoors?.includes(targetObjectDesc)) {
          return 'continue';
        }
        if (isTeleportDoor) {
          this.controller.sendChatMessage(casterActorId, ...this.interiorsBlockedMessage);
          return 'blockActivation';
        }
      }
    }
    return 'continue';
  }

  onPlayerDialogResponse(actorId: number, dialogId: number, buttonIndex: number) {
    if (dialogId === this.joinDeathMatchDialog[0]) {
      if (buttonIndex === 0) {
        const round = getAvailableRound(this.rounds, actorId);
        if (!round || !round.map) {
          // TODO: Handle unavailability to find a round
        }
        else {
          forceJoinRound(this.controller, this.rounds, round, actorId);
        }
      }
    } else if (dialogId === this.leaveRoundConfirmationDialog[0]) {
      const round = getPlayerCurrentRound(this.rounds, actorId);
      if (!round) {
        if (this.rounds[0] && this.rounds[0].hallPointName) {
          this.controller.setSpawnPoint(actorId, this.rounds[0].hallPointName);
          this.controller.teleport(actorId, this.rounds[0].hallPointName);
        }
        return;
      }
      const roundIndex = this.rounds.indexOf(round);
      if (buttonIndex === 0) {
        forceLeaveRound(this.controller, this.rounds, actorId);
      }
      if (round.players?.size === 0) {
        this.resetRound(roundIndex);
      }
    }
    else if (dialogId === this.quitDialog[0]) {
      if (buttonIndex === 0) {
        this.controller.quitGame(actorId);
      }
    }
  }

  onPlayerChatInput(actorId: number, inputText: string, neighbors: number[], senderName: string) {
    for (const neighborActorId of neighbors) {
      this.controller.sendChatMessage(neighborActorId, '#{a8adad}' + senderName + '#{ffffff}: ' + inputText);
    }
  }

  onPlayerJoin(actorId: number) {
    this.controller.setSpawnPoint(actorId, this.hallSpawnPointName);
  }

  everySecond() {
    for (const round of this.rounds) {
      if (round.players && round.players.size) {
        round.secondsPassed = (round.secondsPassed || 0) + 1;
        if (round.state === 'warmup') {
          const secondsRemaining = this.warmupTimerMaximum - round.secondsPassed;
          if (secondsRemaining > 0 && this.sendMessageNeeded(secondsRemaining)) {
            this.sendRoundChatMessage(round, sprintf(this.startingRoundInMessage[0], secondsRemaining));
          }
          if (round.secondsPassed > this.warmupTimerMaximum) {
            round.secondsPassed = 0;
            round.state = 'running';
            this.sendRoundChatMessage(round, sprintf(this.warmupFinishedMessage[0], this.runningTimerMaximum));
            for (const [player] of round.players) {
              if (round.map && round.map.mainSpawnPointName) {
                this.controller.setSpawnPoint(player, round.map.mainSpawnPointName);
                this.controller.teleport(player, round.map.mainSpawnPointName);
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
            const winners = determineDeathMatchWinners(round);
            if (winners.length === 0) {
              this.sendRoundChatMessage(round, ...this.noWinnerMessage);
            }
            else if (winners.length === 1) {
              const winner = winners[0];
              const winnerScore = round.players.get(winner)?.kills;
              this.sendRoundChatMessage(round, sprintf(this.determineWinnerMessage[0], this.controller.getName(winner), winnerScore));
            }
            else {
              this.sendRoundChatMessage(round, ...this.multipleWinnersMessage);
              for (const winner of winners) {
                const winnerScore = round.players.get(winner)?.kills;
                this.sendRoundChatMessage(round, sprintf(this.determineWinnerMessage[0], this.controller.getName(winner), winnerScore));
              };
            }
            this.resetRound(this.rounds.indexOf(round));
          }
        }
      }
    }
  }

  onPlayerDeath(targetActorId: number, killerActorId?: number | undefined) {
    if (killerActorId) {
      const round = getPlayerCurrentRound(this.rounds, targetActorId);
      const round2 = getPlayerCurrentRound(this.rounds, killerActorId);
      if (round === round2 && round && round.players && round.state === 'running') {
        const killerState = round.players.get(killerActorId);
        if (killerState) {
          killerState.kills = (killerState.kills || 0) + 1;
        }
        const killerScore = round.players.get(killerActorId)?.kills;
        const winnerScore = Math.max(...(determineDeathMatchWinners(round).map(x => round.players?.get(x)?.kills) as number[]));
        this.sendRoundChatMessage(round, sprintf(this.deathMessage[0], this.controller.getName(targetActorId), this.controller.getName(killerActorId), this.controller.getName(killerActorId), killerScore, winnerScore));
      }
    }
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
