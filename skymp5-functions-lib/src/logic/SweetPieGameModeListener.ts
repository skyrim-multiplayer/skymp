import { sprintf } from "../lib/sprintf-js";
import { GameModeListener } from "./GameModeListener";
import { PlayerController } from "./PlayerController";
import { SweetPieMap } from "./SweetPieMap";
import { forceLeaveRound, getPlayerCurrentRound, getAvailableRound, forceJoinRound, determineDeathMatchWinners, SweetPieRound } from "./SweetPieRound";

export class SweetPieGameModeListener implements GameModeListener {
  readonly coinFormId = 0xf;
  readonly applePieFormId = 0x00064B43;

  readonly quitGamePortal = '42f3f:SweetPie.esp';
  readonly neutralPortal = '42f70:SweetPie.esp';
  readonly redPortal = '42e96:SweetPie.esp';
  readonly bluePortal = '42fc1:SweetPie.esp';

  readonly noEnterSafePlaceMessage: [string] = ["You cannot go back to safety! Running out of the map is the only option"];
  readonly interiorsBlockedMessage: [string] = ["Interiors are not available during round"];
  readonly warmupFinishedMessage: [string] = ["Warmup finished! Go! You have %d seconds to kill each other!"];
  readonly startingRoundInMessage: [string] = ["Starting round in %d seconds"];
  readonly remainingFightTimeMessage: [string] = ["Fight! You have %d seconds"];
  readonly determineWinnerMessage: [string] = ["%s wins with %d points! Thanks for playing"];
  readonly noWinnerMessage: [string] = ["There is no winner! Thanks for playing"];
  readonly multipleWinnersMessage: [string] = ["We have multiple winners!"];
  readonly deathMessage: [string] = ["%s was slain by %s. %s now has %d points (the best is %d)"];
  readonly restoreMessage: [string] = ["Restored"];
  readonly restoreDeniedMessage: [string] = ["Wait for %d seconds to restore again"];

  readonly cantStartMessage: [string] = ["Too few players, the warmup will start when %s more join"];

  readonly comingSoonPortalName = 'Coming soon...';
  readonly quitGamePortalName = 'Quit the game and return to desktop';
  readonly returnToHallPortalName = 'Return to hall';
  readonly neutralPortalNameTpl = 'Enter deathmatch\nPlayers: %d (min %d)\n%s';

  readonly roundStateToHumanReadable: Record<SweetPieRound['state'], string> = {
    'wait': 'Waiting for players...',
    'warmup': 'Warmup',
    'running': 'Running, please wait',
  };

  warmupTimerMaximum = 60;
  runningTimerMaximum = 300;

  // TODO: Unhardcode this name
  readonly hallSpawnPointName = 'hall:spawnPoint';

  constructor(private controller: PlayerController, private maps: SweetPieMap[] = [], private minimumPlayersToStart: number = 5) {
    this.rounds = this.controller.getRoundsArray();
    if (this.rounds.length === 0) {
      maps.forEach(map => this.rounds.push({ state: 'wait', map: map }));
      this.rounds.forEach((round, index) => this.resetRound(index));
    } else {
      maps.forEach((map, index) => this.rounds[index].map = map);
    }
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
    this.rounds[roundIndex] = { state: 'wait', map: this.rounds[roundIndex].map, hallPointName: this.hallSpawnPointName, secondsPassed: 0 }
    this.controller.setRoundsArray(this.rounds);
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
          this.controller.sendChatMessage(casterActorId, ...this.noEnterSafePlaceMessage);
          return 'blockActivation';
        }
        if (round.map.leaveRoundDoors?.includes(targetObjectDesc)) {
          const roundIndex = this.rounds.indexOf(round);
          forceLeaveRound(this.controller, this.rounds, casterActorId);
          if (round.players?.size === 0) {
            this.resetRound(roundIndex);
            this.controller.setRoundsArray(this.rounds);
          }
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
    for (const neighborActorId of neighbors) {
      this.controller.sendChatMessage(neighborActorId, '' + senderName + ': ' + inputText);
    }
  }

  onPlayerJoin(actorId: number) {
    this.controller.setSpawnPoint(actorId, this.hallSpawnPointName);
  }

  everySecond() {
    for (const round of this.rounds) {
      const playersCount = round.players?.size || 0;
      this.controller.updateCustomName(
        this.neutralPortal,
        sprintf(this.neutralPortalNameTpl, playersCount, this.minimumPlayersToStart, this.roundStateToHumanReadable[round.state]),
      );
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
            } else {
              if (winners.length > 1) {
                this.sendRoundChatMessage(round, ...this.multipleWinnersMessage);
              }
              for (const winner of winners) {
                this.controller.addItem(winner, this.applePieFormId, 1);
                const winnerScore = round.players.get(winner)?.kills;
                this.sendRoundChatMessage(round, sprintf(this.determineWinnerMessage[0], this.controller.getName(winner), winnerScore));
              };
            }
            this.resetRound(this.rounds.indexOf(round));
          }
        }
      }
    }
    this.controller.setRoundsArray(this.rounds);
  }

  onPlayerDeath(targetActorId: number, killerActorId?: number | undefined) {
    if (killerActorId) {
      const round = getPlayerCurrentRound(this.rounds, targetActorId);
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
    this.controller.setRoundsArray(this.rounds);
  }

  onPlayerLeave(actorId: number) {
    const round = getPlayerCurrentRound(this.rounds, actorId);
    forceLeaveRound(this.controller, this.rounds, actorId);
    if (round && round.players?.size === 0) {
      const roundIndex = this.rounds.indexOf(round);
      this.resetRound(roundIndex);
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
