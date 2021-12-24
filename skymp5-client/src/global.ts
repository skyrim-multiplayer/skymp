import { AuthGameData } from "./authModel";

declare global {
  var authData: AuthGameData | undefined;
};
