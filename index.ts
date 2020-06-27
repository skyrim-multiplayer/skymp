import { SkympClient } from './src/skympClient';

import * as networking from './src/networking';
import { printConsole, loadGame, findConsoleCommand, TESModPlatform, Game, on, storage } from 'skyrimPlatform';

new SkympClient;

/*networking.connect('127.0.0.1', 7777);

networking.on('message', (msg: any) => {
    // ...
});

enum MsgType {
    CustomPacket = 1
};

if (typeof storage.hash !== 'string') {
    storage.hash = '';
    for (let i = 0; i < 5; ++i) {
        storage.hash += Math.random().toString(36).substring(6);
    }
}

networking.on('connectionAccepted', () => {
    printConsole('Connected successfully!');
    networking.send({ t: MsgType.CustomPacket, content: { p: 'handshake', hash: storage.hash } }, true);
});

networking.on('connectionDenied', (error: string) => {
    printConsole('Connection denied, reason:', error);
});

networking.on('connectionFailed', () => {
    printConsole('Connection failed');
});

let myActor = 0;

networking.on('message', (msg: any) => {
    switch (msg.type) {
        case 'setUserActor':
            myActor = msg.formId;
            break;
        case 'moveTo':
            if (msg.formId === myActor) {
                loadGame(msg.pos, msg.rot, msg.cellOrWorld);
            }
            break;
    }
    printConsole('Packet from server:', msg);
});*/