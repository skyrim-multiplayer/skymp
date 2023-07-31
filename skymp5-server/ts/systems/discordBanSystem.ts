import { Settings } from "../settings";
import { System, SystemContext } from "./system";
import { Client, ClientOptions, GatewayIntentBits } from "discord.js";

type Mp = any; // TODO

export class DiscordBanSystem implements System {
    systemName = "DiscordBanSystem";

    constructor(
    ) { }

    async initAsync(ctx: SystemContext): Promise<void> {
        let discordAuth = Settings.get().discordAuth;

        if (discordAuth && !discordAuth.botToken) {
            discordAuth = undefined;
            return console.error("discordAuth.botToken is missing, skipping Discord ban system");
        }
        if (discordAuth && !discordAuth.guildId) {
            discordAuth = undefined;
            return console.error("discordAuth.guildId is missing, skipping Discord ban system");
        }
        if (discordAuth && !discordAuth.banRoleId) {
            discordAuth = undefined;
            return console.error("discordAuth.banRoleId is missing, skipping Discord ban system");
        } 

        const client = new Client({ intents: [GatewayIntentBits.Guilds, GatewayIntentBits.GuildMembers] });
        await client.login(discordAuth.botToken);

        client.on("error", (error) => {
            console.error(error);
        });

        client.on("warn", (message) => {
            console.warn(message);
        })

        client.on("guildMemberUpdate", (oldMember, newMember) => {
            // Not sure if it is possible, but better to protect
            if (!oldMember) {
                return console.warn(`oldMember was ${oldMember} in guildMemberUpdate`);
            }
            if (!newMember) {
                return console.warn(`newMember was ${newMember} in guildMemberUpdate`);
            }

            const newRole = newMember.roles.cache
                .filter(r => !oldMember.roles.cache.has(r.id))
                .first();

            if (!newRole) {
                // guildMemberUpdate is also fired on nickname update, role removal, etc
                return;
            }

            if ([newRole.id].indexOf(discordAuth.banRoleId) === -1) {
                return console.log("Detected role add, but not a ban");
            }

            const discordId = newMember.id;

            const mp = ctx.svr as unknown as Mp;
            const forms = mp.findFormsByPropertyValue("private.indexed.discordId", discordId) as number[];

            forms.forEach(formId => {
                console.log(`Detected Discord ban ${formId.toString(16)}, kicking`);
                ctx.svr.setEnabled(formId, false);
            });
        });
    }
}
