import {
  Actor,
  Form,
  Game,
  Utility,
  on,
  once,
  printConsole,
  settings,
} from 'skyrimPlatform';

import { WorldModel } from '../modelSource/model';
import { FormViewArray } from './formViewArray';
import { PlayerCharacterDataHolder } from './playerCharacterDataHolder';
import { View } from './view';

export class WorldView implements View<WorldModel> {
  constructor() {
    // Work around showRaceMenu issue
    // Default nord in Race Menu will have very ugly face
    // If other players are spawning when we show this menu
    on('update', () => {
      const pc = Game.getPlayer() as Actor;
      const pcWorldOrCell = (
        (pc.getWorldSpace() || pc.getParentCell()) as Form
      ).getFormID();
      if (this.pcWorldOrCell !== pcWorldOrCell) {
        if (this.pcWorldOrCell) {
          printConsole('Reset all form views');
          this.formViews.resize(0);
          this.cloneFormViews.resize(0);
        }
        this.pcWorldOrCell = pcWorldOrCell;
      }
    });
    once('update', () => {
      // Wait 1s game time (time spent in Race Menu isn't counted)
      Utility.wait(1).then(() => {
        this.allowUpdate = true;
        printConsole('Update is now allowed');
      });
    });
  }

  getRemoteRefrId(clientsideRefrId: number): number {
    return this.formViews.getRemoteRefrId(clientsideRefrId);
  }

  getLocalRefrId(remoteRefrId: number): number {
    return this.formViews.getLocalRefrId(remoteRefrId);
  }

  update(model: WorldModel): void {
    if (!this.allowUpdate) return;

    const skipUpdates = settings['skymp5-client']['skipUpdates'];

    // skip 50% of updated if said in the settings
    this.counter = !this.counter;
    if (this.counter && skipUpdates) return;

    this.formViews.resize(model.forms.length);

    const showMe = settings['skymp5-client']['show-me'];
    const showClones = settings['skymp5-client']['show-clones'];

    PlayerCharacterDataHolder.updateData();

    this.formViews.updateAll(model, !!showMe, false);

    if (showClones) {
      this.cloneFormViews.updateAll(model, false, true);
    } else {
      this.cloneFormViews.resize(0);
    }
  }

  syncFormArray(model: WorldModel) {
    const showMe = settings['skymp5-client']['show-me'];
    this.formViews.syncFormView(model, !!showMe);
  }

  destroy(): void {
    this.formViews.resize(0);
  }

  private formViews = new FormViewArray();
  private cloneFormViews = new FormViewArray();
  private allowUpdate = false;
  private pcWorldOrCell = 0;
  private counter = false;
}
