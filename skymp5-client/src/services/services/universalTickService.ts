import { ClientListener, CombinedController, Sp } from "./clientListener";

export class UniversalTickService extends ClientListener {
    constructor(private sp: Sp, private controller: CombinedController) {
        super();

        controller.on("update", () => this.onUpdate());
        controller.on("tick", () => this.onTick());

        this.state = { name: "useTick" };
    }

    /**
     * Represents which universal tick is being emitted *right now*.
     * Always "unknown" outside of an emit() call.
     */
    getCurrentUniversalTickType(): "tick" | "update" | "unknown" {
        return this.currentUniversalTickType;
    }

    private onTick(): void {
        switch (this.state.name) {
            case "useTick":
                this.sendUniversalTickEvent("tick");
                break;
            case "useUpdate":
                // Switch to use tick if last update was 2 frames ago
                this.state.tickCounter++;
                if (this.state.updateCounter >= 2) {
                    this.state = { name: "useTick" };
                }
                break;
        }
    }

    private onUpdate(): void {
        switch (this.state.name) {
            case "useTick":
                // Switch to use update
                this.state = { name: "useUpdate", tickCounter: 0, updateCounter: 0 };
                break;
            case "useUpdate":
                this.state.updateCounter++;
                this.sendUniversalTickEvent("update");
                break;
        }
    }

    private sendUniversalTickEvent(type: "tick" | "update") {
        try {
            this.currentUniversalTickType = type;
            this.controller.emitter.emit("universalTick", { type });
        } finally {
            this.currentUniversalTickType = "unknown";
        }
    }

    private state: UniversalTickServiceStateUseUpdate | UniversalTickServiceStateUseTick;
    private currentUniversalTickType: "tick" | "update" | "unknown" = "unknown";
};

interface UniversalTickServiceStateUseUpdate {
    name: "useUpdate";
    tickCounter: number;
    updateCounter: number;
}

interface UniversalTickServiceStateUseTick {
    name: "useTick";
}
