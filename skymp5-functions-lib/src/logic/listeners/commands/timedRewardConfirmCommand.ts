import { Mp } from "../../../types/mp";
import { CombinedController, PlayerController } from "../../PlayerController";
import { SweetTaffyTimedRewards } from "../sweettaffyTimedRewards/SweetTaffyTimedRewards";
import { Command, HandlerInput } from "./command";

export class TimedRewardCommand extends Command {
  constructor(mp: Mp, controller: CombinedController) {
    super(mp, controller, "huntreward");
  }

  handle({ actorId }: HandlerInput): void {
    const timedRewardsListener = this.controller.lookupListener('SweetTaffyTimedRewards') as SweetTaffyTimedRewards;
    const debug = timedRewardsListener.claimAdditionalExtraHourOfGameplayReward(actorId);
    console.log(`${actorId} tried to claim reward: ${debug}`);
  }
}
