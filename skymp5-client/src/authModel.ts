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
  ) { }
}

export class LoginResponseAuthData {
  public constructor(
    public token: string,
    public id: number,
    public name: string,
  ) { }
}

export class AuthGameData {
  public remote?: { session: string, email: string, rememberMe: boolean };
  public local?: { profileId: number };
}
