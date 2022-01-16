import * as sp from "skyrimPlatform";

export interface ScreenResolution {
  Width: number;
  Height: number;
}

let _screenResolution: ScreenResolution | undefined;
export const getScreenResolution = (): ScreenResolution => {
  if (!_screenResolution) {
    _screenResolution = {
      Width: sp.Utility.getINIInt("iSize W:Display"),
      Height: sp.Utility.getINIInt("iSize H:Display"),
    }
  }
  return _screenResolution;
}
