import * as sp from "skyrimPlatform";

export interface ScreenResolution {
  width: number;
  height: number;
}

let _screenResolution: ScreenResolution | undefined;
export const getScreenResolution = (): ScreenResolution => {
  if (!_screenResolution) {
    _screenResolution = {
      width: sp.Utility.getINIInt("iSize W:Display"),
      height: sp.Utility.getINIInt("iSize H:Display"),
    }
  }
  return _screenResolution;
}
