import { ObjectReference } from "@skyrim-platform/skyrim-platform";
import { FormViewArray } from "src/view/formViewArray";
import { FormModel } from "src/view/model";

import * as skyrimPlatform from "skyrimPlatform";

export interface GamemodeApiCtx {
    refr: ObjectReference | undefined;
    value: unknown;
    _model: FormModel | undefined;
    sp: typeof skyrimPlatform,
    state: Record<string, unknown> | undefined;
    _view: FormViewArray | undefined;
    i: number;
    getFormIdInServerFormat: (clientsideFormId: number) => number;
    getFormIdInClientFormat: (serversideFormId: number) => number;
    get: (propName: string) => unknown;
    respawn: () => void;
}
