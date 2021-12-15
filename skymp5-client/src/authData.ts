export class AuthData {
  public static canRegister(data: AuthData) {
    return data && data.name?.trim() && data.email?.trim() && data.password?.trim();
  }
  public static canLogin(data: AuthData) {
    return data && data.email?.trim() && data.password?.trim();
  }

  public constructor(
    public name: string,
    public email: string,
    public password: string,
    public rememberMe?: boolean,
  ) { }
}

export class TokenAuthData {
  public constructor(
    public token: string,
    public id: number,
    public name: string,
  ) { }
}
