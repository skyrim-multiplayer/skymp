import { DxScanCode } from "@skyrim-platform/skyrim-platform";

export interface QueryKeyCodeBindings {
    isDown: (binding: DxScanCode[]) => boolean;
}
