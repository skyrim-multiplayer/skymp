import * as sp from 'skyrimPlatform';
import { PacketType, mpClientPlugin } from 'skyrimPlatform';

type Handler = (messageOrError: Record<string, unknown> | string) => void;
const handlersMap = new Map<PacketType, Handler[]>();
let lastHostname = '';
let lastPort = 0;

const createClientSafe = (hostname: string, port: number): void => {
  sp.printConsole('createClientSafe ' + hostname + ':' + port);
  // Client sometimes call this function with bad parameters.
  // It causes assertion failure in Debug mode, but doesn't lead to anything on a regular player's machine.
  // It seems that this function will be called with the valid parameters later
  if (hostname !== '' && lastPort !== 0) {
    mpClientPlugin.createClient(hostname, port);
  }
};

sp.on('tick', () => {
  mpClientPlugin.tick((packetType, jsonContent, error) => {
    const handlers = handlersMap.get(packetType) || [];
    handlers.forEach((handler) => {
      const parse = () => {
        try {
          return JSON.parse(jsonContent);
        } catch (e) {
          throw new Error(`JSON ${jsonContent} failed to parse: ${e}`);
        }
      };
      handler(jsonContent.length ? parse() : error);
    });
  });
});

export const connect = (hostname: string, port: number): void => {
  lastHostname = hostname;
  lastPort = port;
  createClientSafe(hostname, port);
};

export const close = (): void => {
  mpClientPlugin.destroyClient();
};

export const on = (packetType: PacketType, handler: Handler): void => {
  let arr = handlersMap.get(packetType);
  arr = (arr ? arr : []).concat([handler]);
  handlersMap.set(packetType, arr);
};

export const send = (msg: Record<string, unknown>, reliable: boolean): void => {
  // TODO(#175): JS object instead of JSON?
  mpClientPlugin.send(JSON.stringify(msg), reliable);
};

// Reconnect automatically
export const reconnect = (): void => createClientSafe(lastHostname, lastPort);
on('connectionFailed', reconnect);
on('connectionDenied', reconnect);
on('disconnect', reconnect);
