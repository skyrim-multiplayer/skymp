import { ClientListener, Sp, CombinedController } from "./clientListener";

export class DisableDifficultySelectionService extends ClientListener {
    constructor(private sp: Sp, private controller: CombinedController) {
        super();
        this.controller.on("update", () => this.onUpdate());
    }

    private onUpdate() {
        this.counter++;
        if (this.counter >= 60) {
            this.counter = 0;
            this.sp.Utility.setINIInt("iDifficulty:GamePlay", this.difficulty);
        }
    }

    private readonly difficulty = 5;

    private counter = 0;
}
