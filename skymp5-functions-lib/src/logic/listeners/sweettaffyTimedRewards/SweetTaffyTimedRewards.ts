import { Inventory, LocationalData } from "../../../types/mp";
import { Position, squareDist } from "../../../utils/locationUtils";
import { GameModeListener, ListenerLookupController } from "../GameModeListener";
import { Counter } from "../../PlayerController";
import { ChatMessage, createSystemMessage } from "../../../props/chatProperty";

export type TimedRewardController = {
  getCurrentTime(): Date;
  getOnlinePlayers(): number[];
  addItem(actorId: number, itemId: number, count: number): void;
  setCounter(actorId: number, counter: Counter, to: number): void;
  getCounter(actorId: number, counter: Counter): number;
  getInventory(actorId: number): Inventory;
  getLocation(actorId: number): LocationalData;
  sendChatMessage(actorId: number, message: ChatMessage): void;
}

export type BiomeName = string;

export type BiomeInfo = {
  name: BiomeName;
  pos: [number, number, number];
};

export type RewardRule = {
  itemFormId: number;
  itemCountWeights: number[];

  biome?: BiomeName;
  requiredItemFormId?: number;
}

export type TimedRewardConfig = {
  // enableDaily, enablyHourly are here to simplify tests
  enableDaily: boolean;
  enableHourly: boolean;
  rules?: RewardRule[];
  biomes?: BiomeInfo[];
}

export function getRandomIntByWeights(weights: number[]): number {
  let total = 0;
  for (const w of weights) {
    total += w;
  }
  let choice = Math.floor(Math.random() * total);
  for (let i = 0; i < weights.length; ++i) {
    if (choice < weights[i]) {
      return i;
    }
    choice -= weights[i];
  }
  return 0;
}

export function dayStart(date: Date): Date {
  const utcOffset = 1000 * 60 * 60 * 3; // 00:00 UTC+3
  const utcStart = new Date(date.getTime() + utcOffset);
  utcStart.setUTCHours(0, 0, 0, 0);
  return new Date(utcStart.getTime() - utcOffset);
}

export class SweetTaffyTimedRewards implements GameModeListener {
  static rewardItemFormId = 0x07F33922;

  constructor(private controller: ListenerLookupController & TimedRewardController, private config: TimedRewardConfig) {
    controller.registerListenerForLookup('SweetTaffyTimedRewards', this);
  }

  everySecond() {
    const currentTime = this.controller.getCurrentTime();
    const todayStart = dayStart(currentTime).getTime();
    for (const playerActorId of this.controller.getOnlinePlayers()) {
      if (this.config.enableDaily) {
        const lastDay = this.controller.getCounter(playerActorId, 'everydayStart');
        if (lastDay === 0) {
          this.controller.setCounter(playerActorId, 'everydayStart', todayStart);
          this.controller.setCounter(playerActorId, 'secondsToday', 0);
          this.controller.setCounter(playerActorId, 'lastExtraRewardDay', todayStart);
          this.controller.addItem(playerActorId, SweetTaffyTimedRewards.rewardItemFormId, 150);
        } else {
          const fullDays = Math.floor((currentTime.getTime() - lastDay) / (1000 * 60 * 60 * 24));
          if (fullDays > 0) {
            this.controller.setCounter(playerActorId, 'everydayStart', todayStart);
            this.controller.setCounter(playerActorId, 'secondsToday', 0);
            this.controller.addItem(playerActorId, SweetTaffyTimedRewards.rewardItemFormId, fullDays);
          }
        }
      }

      if (this.config.enableHourly) {
        const secondsToday = this.controller.getCounter(playerActorId, 'secondsToday') + 1;
        this.controller.setCounter(playerActorId, 'secondsToday', secondsToday);
        if (secondsToday == 60 * 60 && this.controller.getCounter(playerActorId, 'lastExtraRewardDay') !== todayStart) {
          this.controller.setCounter(playerActorId, 'lastExtraRewardDay', todayStart);
          this.controller.addItem(playerActorId, SweetTaffyTimedRewards.rewardItemFormId, 9);
          this.notifyClaimableIfHasMatchingRules(playerActorId);
        }
      }
    }
  }

  notifyClaimableIfHasMatchingRules(playerActorId: number) {
    for (const rule of this.config.rules || []) {
      if (!rule.requiredItemFormId || this.playerHasItem(playerActorId, rule.requiredItemFormId)) {
        this.controller.sendChatMessage(playerActorId, createSystemMessage([
          {text: `Чутьё охотника подсказывает вам, что самое время проверить ловушки. Быть может, сегодня удачный день?`, color: '#FFFFFF', type: ['plain']},
          {text: '\nИспользуйте команду /huntreward', color: '#91916D', type: ['plain']},
        ]));
        break;
      }
    }
  }

  claimAdditionalExtraHourOfGameplayReward(playerActorId: number) {
    const currentTime = this.controller.getCurrentTime();
    const todayStart = dayStart(currentTime).getTime();
    const lastExtraRewardDay = this.controller.getCounter(playerActorId, 'lastExtraRewardDay');
    const lastClaimedExtraRewardDay = this.controller.getCounter(playerActorId, 'lastClaimedExtraRewardDay');
    if (todayStart === lastClaimedExtraRewardDay) {
      this.controller.sendChatMessage(playerActorId, createSystemMessage(`То ли из отчаяния, то ли из любопытства вы решили вновь проверить свои ловушки. Но никто не пришёл.`));
      return 'already_claimed';
    }

    if (lastExtraRewardDay !== todayStart) {
      // not available for claiming yet
      const secondsToday = this.controller.getCounter(playerActorId, 'secondsToday');
      const minutesLeft = Math.ceil((60 * 60 - secondsToday) / 60);
      const minutesLeftRounded = Math.ceil(minutesLeft / 5) * 5;
      this.controller.sendChatMessage(playerActorId, createSystemMessage([
        {text: 'Вы думаете что еще рано проверять ловушки. ', color: '#FFFFFF', type: ['plain']},
        {text: `\nБыть может, подождать ещё хотя бы ${minutesLeftRounded} минут?`, color: '#91916D', type: ['plain']},
      ]));
      return minutesLeft;
    }

    const playerBiome = this.getPlayerBiome(playerActorId);
    if (playerBiome === '') {
      this.controller.sendChatMessage(playerActorId, createSystemMessage([
        {text: 'Единственные ловушки, которые вы видите, расставленные повсюду - ночные горшки и швабры. Вероятно, мёд с прошлой попойки ещё не выветрился...', color: '#FFFFFF', type: ['plain']},
        {text: '\nОхотиться можно только в Скайриме. Выйдите на улицу, если вы находитесь в закрытой локации.', color: '#91916D', type: ['plain']},
      ]));
      return 'wrong_cell';
    }

    this.controller.setCounter(playerActorId, 'lastClaimedExtraRewardDay', todayStart);

    const debug = [];
    for (const rule of this.config.rules || []) {
      debug.push(this.giveRewardByRule(playerActorId, playerBiome, rule));
    }
    console.log(`player actorId=${playerActorId.toString(16)} biome=${playerBiome} rule outcomes: ${debug}`);
    return 0;
  }

  getPlayerBiome(playerActorId: number): BiomeName {
    const biomes = this.config.biomes || [];
    if (biomes.length === 0) {
      console.log(`getPlayerBiome(${playerActorId.toString(16)}): empty biome config`)
      return '';
    }
    const { cellOrWorldDesc: playerCell, pos: playerPos } = this.controller.getLocation(playerActorId);
    console.log(playerCell, playerPos);
    if (playerCell !== '3c:Skyrim.esm') {
      console.log(`getPlayerBiome(${playerActorId.toString(16)}) @ ${playerCell} ${playerPos}: not main cell`)
      return '';
    }
    const getSquareDist = (biomePos: Position) => squareDist(playerPos, biomePos);
    let closestBiomeIdx = 0, closestBiomeSquareDist = getSquareDist(biomes[0].pos);
    for (let i = 1; i < biomes.length; ++i) {
      const currentBiomeSquareDist = getSquareDist(biomes[i].pos);
      if (currentBiomeSquareDist < closestBiomeSquareDist) {
        closestBiomeIdx = i;
        closestBiomeSquareDist = currentBiomeSquareDist;
      }
    }
    console.log(`getPlayerBiome(${playerActorId.toString(16)}) @ ${playerCell} ${playerPos}: ${biomes[closestBiomeIdx].name}`)
    return biomes[closestBiomeIdx].name;
  }

  playerHasItem(playerActorId: number, itemFormId: number) {
    for (const entry of this.controller.getInventory(playerActorId).entries) {
      if (entry.baseId === itemFormId) {
        return true;
      }
    }
    return false;
  }

  giveRewardByRule(playerActorId: number, playerBiome: BiomeName, rule: RewardRule) {
    if (rule.biome && rule.biome !== playerBiome) {
      return 'other_biome';
    }
    if (rule.requiredItemFormId && !this.playerHasItem(playerActorId, rule.requiredItemFormId)) {
      return 'no_required_item';
    }
    const itemCount = getRandomIntByWeights(rule.itemCountWeights);
    if (itemCount === 0) {
      return 'rolled_0';
    }
    this.controller.addItem(playerActorId, rule.itemFormId, itemCount);
    return itemCount;
  }
}
