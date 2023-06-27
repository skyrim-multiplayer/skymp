export class RemoteAuthGameData {
  public constructor(
    public session: string,
    public masterApiId: number,
    public discordUsername: string | null,
    public discordDiscriminator: string | null,
    public discordAvatar: string | null,
  ) {}
}

export class AuthGameData {
  public static readonly storageKey = 'authGameData';

  public remote?: RemoteAuthGameData;
  public local?: { profileId: number };
}
