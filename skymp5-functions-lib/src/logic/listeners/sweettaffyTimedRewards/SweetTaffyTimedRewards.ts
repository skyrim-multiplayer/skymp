import { GameModeListener } from "../gameModeListener";
import { Counter } from "../../PlayerController";

export type TimedRewardController = {
  getCurrentTime(): Date;
  getOnlinePlayers(): number[];
  addItem(actorId: number, itemId: number, count: number): void;
  setCounter(actorId: number, counter: Counter, to: number): void;
  getCounter(actorId: number, counter: Counter): number;
}

export function dayStart(date: Date): Date {
  const utcOffset = 1000 * 60 * 60 * 3; // 00:00 UTC+3
  const utcStart = new Date(date.getTime() + utcOffset);
  utcStart.setUTCHours(0, 0, 0, 0);
  return new Date(utcStart.getTime() - utcOffset);
}

export class SweetTaffyTimedRewards implements GameModeListener {
  static rewardItemFormId = 0x07F33922;

  // enableDaily, enablyHourly are here to simplify tests
  constructor(private controller: TimedRewardController, private enableDaily: boolean, private enableHourly: boolean) {
  }

  everySecond() {
    const currentTime = this.controller.getCurrentTime();
    const todayStart = dayStart(currentTime).getTime();
    for (const playerActorId of this.controller.getOnlinePlayers()) {
      if (this.enableDaily) {
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

      if (this.enableHourly) {
        const secondsToday = this.controller.getCounter(playerActorId, 'secondsToday') + 1;
        this.controller.setCounter(playerActorId, 'secondsToday', secondsToday);
        if (secondsToday == 60 * 60 && this.controller.getCounter(playerActorId, 'lastExtraRewardDay') !== todayStart) {
          this.controller.setCounter(playerActorId, 'lastExtraRewardDay', todayStart);
          this.controller.addItem(playerActorId, SweetTaffyTimedRewards.rewardItemFormId, 9);
        }
      }
    }
  }
}
