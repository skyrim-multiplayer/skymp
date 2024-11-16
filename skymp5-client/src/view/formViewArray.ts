import { FormView } from "./formView";
import { FormModel, WorldModel } from "./model";
import { NiPoint3 } from "../sync/movement";
import { SpApiInteractor } from "../services/spApiInteractor";
import { GamemodeUpdateService } from "../services/services/gamemodeUpdateService";

export class FormViewArray {
  updateForm(form: FormModel, i: number) {
    const view = this.formViews[i];
    if (!view) {
      this.formViews[i] = new FormView(form.refrId);
    } else {
      view.update(form);
    }
  }

  destroyForm(i: number) {
    const formView = this.formViews[i];
    if (formView === undefined) return;

    formView.destroy();
    this.formViews[i] = undefined;
  }

  resize(newSize: number) {
    if (this.formViews.length > newSize) {
      this.formViews.slice(newSize).forEach((v) => v && v.destroy());
    }
    this.formViews.length = newSize;
  }

  updateAll(model: WorldModel, showMe: boolean, isCloneView: boolean) {
    const controller = SpApiInteractor.getControllerInstance();

    const gamemodeUpdateService = controller.lookupListener(GamemodeUpdateService);
    gamemodeUpdateService.setFormViewArray(this);

    this.isSyncSuspended = false;
    controller.emitter.emit("querySuspendSync", {
      suspend: () => {
        this.isSyncSuspended = true;
      }
    });

    const forms = model.forms;
    const n = forms.length;
    for (let i = 0; i < n; ++i) {
      const form = forms[i];

      if (form === undefined) {
        this.destroyForm(i);
        continue;
      }

      if (this.isSyncSuspended) {
        this.destroyForm(i);
        continue;
      }

      if (model.playerCharacterFormIdx === i && !showMe) {
        this.destroyForm(i);
        continue;
      }

      let realPos: NiPoint3 | undefined = undefined;
      const offset = model.playerCharacterFormIdx === i || isCloneView;

      if (offset && form.movement) {
        realPos = form.movement.pos;
        form.movement.pos = [
          realPos[0] + 128,
          realPos[1] + 128,
          realPos[2],
        ];
      }

      if (isCloneView) {
        // Prevent using the same refr by normal and clone views
        if (!form.refrId || form.refrId >= 0xff000000) {
          const backup = form.isHostedByOther;
          form.isHostedByOther = true;
          // TODO: Explain why do not GamemodeApiSupport.setI(i); here
          this.updateForm(form, i);
          form.isHostedByOther = backup;
        }
      } else {
        gamemodeUpdateService.setI(i);
        this.updateForm(form, i);
      }

      if (offset && form.movement && realPos) {
        form.movement.pos = realPos;
      }
    }
  }

  syncFormView(model: WorldModel, showMe: boolean,) {
    for (let i = 0; i < model.forms.length; ++i) {
      if (!model.forms[i] || (model.playerCharacterFormIdx === i && !showMe)) {
        this.destroyForm(i);
        continue;
      }
    }
  }

  getRemoteRefrId(clientsideRefrId: number): number {
    if (clientsideRefrId < 0xff000000)
      throw new Error("This function is only for 0xff forms");
    const formView = this.formViews.find((formView?: FormView) => {
      return formView && formView.getLocalRefrId() === clientsideRefrId;
    });
    return formView ? formView.getRemoteRefrId() : 0;
  }

  getLocalRefrId(remoteRefrId: number): number {
    if (remoteRefrId < 0xff000000)
      throw new Error("This function is only for 0xff forms");
    const formView = this.formViews.find((formView?: FormView) => {
      return formView && formView.getRemoteRefrId() === remoteRefrId;
    });
    return formView ? formView.getLocalRefrId() : 0;
  }

  getNthFormView(i: number): FormView | undefined {
    return this.formViews[i];
  }

  getFormViewsArrayLength(): number {
    return this.formViews.length;
  }

  private formViews = new Array<FormView | undefined>();
  private isSyncSuspended = false;
}
