export class LoginRegisterData {
  public static canRegister(data: LoginRegisterData) {
    return data && data.email?.trim() && data.password?.trim();
  }
  public static canLogin(data: LoginRegisterData) {
    return data && data.email?.trim() && data.password?.trim();
  }

  public constructor(
    public name: string,
    public email: string,
    public password: string,
    public passwordRepeat?: string,
    public rememberMe?: boolean,
    public changed?: boolean,
  ) { }
}

export class LoginResponseAuthData {
  public constructor(
    public token: string,
    public id: number,
    public name: string,
  ) { }
}

export class RemoteAuthGameData {
  public constructor(
    public session: string,
    public masterApiId: number,
    public discordUsername: string | null,
    public discordDiscriminator: string | null,
    public discordAvatar: string | null,
  ) { }
}

export class AuthGameData {
  public static readonly storageKey = "authGameData";

  public remote?: RemoteAuthGameData;
  public local?: { profileId: number };
}
