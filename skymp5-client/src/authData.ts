export class AuthData {
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
