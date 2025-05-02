import { Settings } from "../settings";
import { Content, System, SystemContext } from "./system";

const fs = require('fs');
const path = require('path');
const express = require('express');


export class LoginDiscord implements System {
  systemName = "Login";

  private settingsObject: Settings;
  private app: any;
  async initAsync(ctx: SystemContext): Promise<void> {
    this.settingsObject = await Settings.get();

    this.app = express({ port: 3001 });

    this.app.get('/api/auth/callback/discord', async ({ query }, response) => {
      const { code } = query;

      if (code) {
        try {
          const tokenResponseData = await fetch('https://discord.com/api/oauth2/token', {
            method: 'POST',
            body: new URLSearchParams({
              client_id: "1351639307839864893",
              client_secret: "hGHhFa2RNWsXL4hNAMtQf0Hxh5ktt5mb",
              code,
              grant_type: 'authorization_code',
              redirect_uri: `http://78.46.181.76:3001/api/auth/callback/discord`,
              scope: 'identify',
            }).toString(),
            headers: {
              'Content-Type': 'application/x-www-form-urlencoded',
            },
          });

          const oauthData = await tokenResponseData.json();

          return response.redirect(`skyrim-rp://auth?access_token=${oauthData.access_token}`);
        } catch (error) {
          // NOTE: An unauthorized token will not throw an error
          // tokenResponseData.statusCode will be 401
          console.error(error);
          return response.send("OK");
        }
      }

      return response.send("OK");
    });

    this.app.listen(3001, () => console.log(`[+] Login Discord OAuth listening at http://localhost:${3001}`));
  }

  customPacket(
    userId: number,
    type: string,
    content: Content,
    ctx: SystemContext
  ): void {
    const ip = ctx.svr.getUserIp(userId);
    console.log(`Connecting a user ${userId} with ip ${ip}`);

    // Get the profileId (Discord access token) from gameData
    const gameData = content["gameData"];
    const accessToken = gameData?.accessToken;

    // Path to the JSON file storing profileIds
    const profilesFilePath = path.join(__dirname, 'profiles.json');

    // Create profiles file if it doesn't exist with initial structure
    if (!fs.existsSync(profilesFilePath)) {
      fs.writeFileSync(profilesFilePath, JSON.stringify({
        lastIndex: 0,
        users: {}
      }));
    }

    // If no access token provided, reject the connection
    if (!accessToken) {
      console.error('No access token provided');
      return;
    }

    // Use fetch to verify the Discord token and get user info
    fetch('https://discord.com/api/users/@me', {
      headers: {
        Authorization: `Bearer ${accessToken}`
      }
    })
      .then(response => {
        if (!response.ok) {
          throw new Error(`Discord API responded with status: ${response.status}`);
        }
        return response.json();
      })
      .then(userData => {
        const discordUserId = userData.id;

        // Load existing profiles
        let profilesData = JSON.parse(fs.readFileSync(profilesFilePath, 'utf8'));

        // Make sure we have the expected structure
        if (!profilesData.users) {
          profilesData = {
            lastIndex: 0,
            users: {}
          };
        }

        // Check if the Discord user already has a profileId
        if (profilesData.users[discordUserId] !== undefined) {
          const profileId = profilesData.users[discordUserId];
          console.log(`Verified user ${discordUserId}. Using stored profileId: ${profileId}`);

          // Return the stored profileId for the verified user
          this.emit(ctx, "spawnAllowed", userId, profileId, [], undefined);
        } else {
          // Increment the last used index
          profilesData.lastIndex += 1;
          const profileId = profilesData.lastIndex;

          // Save the new profileId to the JSON file
          profilesData.users[discordUserId] = profileId;
          fs.writeFileSync(profilesFilePath, JSON.stringify(profilesData, null, 2));
          console.log(`Verified new Discord user ${discordUserId}. Created profileId: ${profileId}`);

          this.emit(ctx, "spawnAllowed", userId, profileId, [], undefined);
        }
      })
      .catch(error => {
        console.error('Error verifying Discord token:', error);
        // Optional: You could reject the connection here instead of using a temp profileId
        const tempProfileId = -1; // Using -1 for temporary profiles
        console.log(`Failed to verify user. Using temporary profileId: ${tempProfileId}`);
        this.emit(ctx, "spawnAllowed", userId, tempProfileId, [], undefined);
      });
  }

  private emit(ctx: SystemContext, eventName: string, ...args: unknown[]) {
    (ctx.gm as any).emit(eventName, ...args);
  }
}
