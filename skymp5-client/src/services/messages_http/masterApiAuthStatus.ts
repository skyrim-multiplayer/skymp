export interface MasterApiAuthStatus {
    token: string;
    masterApiId: number;
    discordUsername: string | null;
    discordDiscriminator: string | null;
    discordAvatar: string | null;
}
