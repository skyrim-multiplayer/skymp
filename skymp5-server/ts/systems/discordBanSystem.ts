import { Settings } from '../settings';
import { System, SystemContext } from './system';

type Mp = any; // TODO

export class DiscordBanSystem implements System {
  systemName = 'DiscordBanSystem';

  constructor() {}

  async initAsync(ctx: SystemContext): Promise<void> {
    const settingsObject = await Settings.get();

    const discordAuth = settingsObject.discordAuth;

    if (settingsObject.offlineMode) {
      return console.log('discord ban system is disabled due to offline mode');
    }
    if (!discordAuth) {
      return console.warn(
        'discordAuth is missing, skipping Discord ban system',
      );
    }
    if (!discordAuth.botToken) {
      return console.warn(
        'discordAuth.botToken is missing, skipping Discord ban system',
      );
    }
    if (!discordAuth.guilds || discordAuth.guilds.length === 0) {
      return console.warn(
        'discordAuth.guilds array is empty or missing, skipping Discord ban system',
      );
    }

    // Load discord.js only when the feature is actually enabled.
    // This avoids pulling optional dependency trees in offline mode.
    const { Client, GatewayIntentBits } = await import('discord.js');
    const client = new Client({
      intents: [GatewayIntentBits.Guilds, GatewayIntentBits.GuildMembers],
    });

    try {
      await client.login(discordAuth.botToken);
    } catch (e) {
      return console.error(`Error logging in Discord client: ${e}`);
    }

    client.on('error', (error) => {
      console.error(error);
    });

    client.on('warn', (message) => {
      console.warn(message);
    });

    client.on('guildMemberUpdate', (oldMember, newMember) => {
      if (!oldMember || !newMember) {
        return;
      }

      const guildConfig = discordAuth.guilds.find(
        (g) => g.guildId === newMember.guild.id,
      );

      if (!guildConfig || !guildConfig.banRoleId) {
        return;
      }

      const newRole = newMember.roles.cache
        .filter((r) => !oldMember.roles.cache.has(r.id))
        .first();

      if (!newRole) {
        return;
      }

      if (newRole.id !== guildConfig.banRoleId) {
        return;
      }

      const discordId = newMember.id;

      const mp = ctx.svr as unknown as Mp;
      const forms = mp.findFormsByPropertyValue(
        'private.indexed.discordId',
        discordId,
      ) as number[];

      forms.forEach((formId) => {
        console.log(
          `Detected Discord ban on guild ${newMember.guild.id}, kicking ${formId.toString(16)}`,
        );
        ctx.svr.setEnabled(formId, false);
      });
    });
  }
}
