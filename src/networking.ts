import { mpClientPlugin, printConsole, PacketType } from 'skyrimPlatform';
import * as sp from 'skyrimPlatform';

mpClientPlugin.destroyClient();

type Handler = (messageOrError: object | string) => void;
let handlersMap = new Map<PacketType, Handler[]>();
let lastHostname = '';
let lastPort = 0;

sp.on('tick', () => {
    mpClientPlugin.tick((packetType, jsonContent, error) => {
        let handlers = handlersMap.get(packetType) || [];
        handlers.forEach((handler) => {
            let parse = (() => {
                try {
                    return JSON.parse(jsonContent);
                }
                catch(e) {
                    throw new Error(`JSON ${jsonContent} failed to parse: ${e}`);
                }
            });
            handler(jsonContent.length ? parse() : error);
        });
    });
});

export let connect = (hostname: string, port: number) => {
    lastHostname = hostname;
    lastPort = port;
    mpClientPlugin.createClient(hostname, port);
};

export let on = (packetType: PacketType, handler: Handler) => {
    let arr = handlersMap.get(packetType);
    arr = (arr ? arr : []).concat([handler]);
    handlersMap.set(packetType, arr);
};

export let send = (msg: object, reliable: boolean) => {
    mpClientPlugin.send(JSON.stringify(msg), reliable);
};

// Reconnect automatically
let reconnect = () => mpClientPlugin.createClient(lastHostname, lastPort);
on('connectionFailed', reconnect);
on('connectionDenied', reconnect);
on('disconnect', reconnect);