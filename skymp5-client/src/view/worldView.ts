import { Form } from "skyrimPlatform";

import { WorldModel } from './model';
import { FormViewArray } from './formViewArray';
import { PlayerCharacterDataHolder } from './playerCharacterDataHolder';
import { ClientListener, CombinedController, Sp } from '../services/services/clientListener';
import { logTrace } from "../logging";
import { SinglePlayerService } from "../services/services/singlePlayerService";
import { RemoteServer } from "../services/services/remoteServer";

export class WorldView extends ClientListener {
  constructor(private sp: Sp, private controller: CombinedController) {
    super();

    controller.on("update", () => this.onUpdate());
    controller.once("update", () => this.onceUpdate());

    this.state = this.makeEmptyState();

    const oldView = this.sp.storage["view"];

    // can't use instanceof here because each hot reload creates a new class
    this.oldView = typeof oldView === "object" ? oldView as WorldView : undefined;

    this.sp.storage["view"] = this;
  }

  getRemoteRefrId(clientsideRefrId: number): number {
    return this.state.formViews.getRemoteRefrId(clientsideRefrId);
  }

  getLocalRefrId(remoteRefrId: number): number {
    return this.state.formViews.getLocalRefrId(remoteRefrId);
  }

  syncFormArray(model: WorldModel) {
    const { settings } = this.sp;
    const showMe = settings['skymp5-client']['show-me'];
    this.state.formViews.syncFormView(model, !!showMe);
  }

  destroy() {
    this.state.formViews.resize(0);
    this.state.cloneFormViews.resize(0); // Recenrly added, not tested if it's needed
    this.state = this.makeEmptyState();
  }

  getFormViews() {
    return this.state.formViews;
  }

  private onUpdate() {
    this.resetAllFormViewsIfPlayerChangedWorld();

    const singlePlayerService = this.controller.lookupListener(SinglePlayerService);
    if (!singlePlayerService.isSinglePlayer) {
      const modelSource = this.controller.lookupListener(RemoteServer);
      this.updateWorld(modelSource.getWorldModel());
    }
  }

  private onceUpdate() {
    if (this.oldView) {
      this.oldView.destroy();
      this.oldView = undefined;
      logTrace(this, 'Previous View destroyed');
    }
    this.waitOneSecondAndAllowFormViewUpdate();
  }

  private resetAllFormViewsIfPlayerChangedWorld() {
    const state = this.state;
    const pc = this.sp.Game.getPlayer()!;
    const pcWorldOrCell = (
      (pc.getWorldSpace() || pc.getParentCell()) as Form
    ).getFormID();
    if (state.pcWorldOrCell !== pcWorldOrCell) {
      if (state.pcWorldOrCell) {
        logTrace(this, 'Reset all form views');
        state.formViews.resize(0);
        state.cloneFormViews.resize(0);
      }
      state.pcWorldOrCell = pcWorldOrCell;
    }
  }

  // Work around showRaceMenu issue
  // Default nord in Race Menu will have very ugly face
  // If other players are spawning when we show this menu
  // TODO: separate listener
  private waitOneSecondAndAllowFormViewUpdate() {
    // Wait 1s game time (time spent in Race Menu isn't counted)
    this.sp.Utility.wait(1).then(() => {
      this.state.allowUpdate = true;
      logTrace(this, 'Update is now allowed');
    });
  }

  private updateWorld(model: WorldModel): void {
    const { settings } = this.sp;
    const state = this.state;

    if (!state.allowUpdate) return;

    const skipUpdates = settings['skymp5-client']['skipUpdates'];

    // skip 50% of updates if specified in the settings
    state.counter = !state.counter;
    if (state.counter && skipUpdates) return;

    state.formViews.resize(model.forms.length);

    const showMe = settings['skymp5-client']['show-me'];
    const showClones = settings['skymp5-client']['show-clones'];

    PlayerCharacterDataHolder.updateData();

    state.formViews.updateAll(model, !!showMe, false);

    if (showClones) {
      state.cloneFormViews.updateAll(model, false, true);
    } else {
      state.cloneFormViews.resize(0);
    }
  }

  private makeEmptyState() {
    return {
      formViews: new FormViewArray(),
      cloneFormViews: new FormViewArray(),
      allowUpdate: false,
      pcWorldOrCell: 0,
      counter: false,
    }
  }

  private state: {
    formViews: FormViewArray;
    cloneFormViews: FormViewArray;
    allowUpdate: boolean;
    pcWorldOrCell: number;
    counter: boolean;
  };

  private oldView?: WorldView;
}
