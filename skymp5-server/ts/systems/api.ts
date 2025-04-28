import { System, Log, SystemContext } from "./system";
import { ScampServer } from "../scampNative";

const express = require('express');
const cors = require('cors')

export class Api implements System {
  systemName = "Api";
  port = 3002;
  app: any;

  constructor(
    private log: Log,
    private maxPlayers: number,
  ) { }

  async initAsync(ctx: SystemContext): Promise<void> {
    this.app = express({ port: this.port });
    this.app.use(cors());

    this.app.get('/api/server-info', async ({ query }: any, response: any) => {
      const online = this.getCurrentOnline(ctx.svr);
      const maxPlayers = this.maxPlayers;

      return response.send(JSON.stringify({ online: online, maxPlayers: maxPlayers }));
    });

    this.app.listen(this.port, () => console.log(`[+] API Running on http://localhost:${this.port}`));
  }

  update(): void {
    return;
  }

  // connect/disconnect events are not reliable so we do full recalculate
  private getCurrentOnline(svr: ScampServer): number {
    return (svr as any).get(0, "onlinePlayers").length;
  }
}
