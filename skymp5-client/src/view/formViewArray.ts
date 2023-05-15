import { FormView } from "./formView";
import { GamemodeApiSupport } from "../gamemodeApi/gamemodeApiSupport";
import { FormModel, WorldModel } from "../modelSource/model";
import { Movement, NiPoint3 } from "../sync/movement";

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
    if (!this.formViews[i]) return;
    this.formViews[i].destroy();
    this.formViews[i] = undefined as unknown as FormView;
  }

  resize(newSize: number) {
    if (this.formViews.length > newSize) {
      this.formViews.slice(newSize).forEach((v) => v && v.destroy());
    }
    this.formViews.length = newSize;
  }

  updateAll(model: WorldModel, showMe: boolean, isCloneView: boolean) {
    GamemodeApiSupport.setFormViewArray(this);
    const forms = model.forms;
    const n = forms.length;
    for (let i = 0; i < n; ++i) {
      if (!forms[i] || (model.playerCharacterFormIdx === i && !showMe)) {
        this.destroyForm(i);
        continue;
      }
      const form = forms[i];

      let realPos: NiPoint3 = undefined as unknown as NiPoint3;
      const offset =
        form.movement && (model.playerCharacterFormIdx === i || isCloneView);
      if (offset) {
        realPos = (form.movement as Movement).pos;
        (form.movement as Movement).pos = [
          realPos[0] + 64,
          realPos[1] + 64,
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
        GamemodeApiSupport.setI(i);
        this.updateForm(form, i);
      }

      if (offset) {
        (form.movement as Movement).pos = realPos as NiPoint3;
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
    const formView = this.formViews.find((formView: FormView) => {
      return formView && formView.getLocalRefrId() === clientsideRefrId;
    });
    return formView ? formView.getRemoteRefrId() : 0;
  }

  getLocalRefrId(remoteRefrId: number): number {
    if (remoteRefrId < 0xff000000)
      throw new Error("This function is only for 0xff forms");
    const formView = this.formViews.find((formView: FormView) => {
      return formView && formView.getRemoteRefrId() === remoteRefrId;
    });
    return formView ? formView.getLocalRefrId() : 0;
  }

  private formViews = new Array<FormView>();
}
