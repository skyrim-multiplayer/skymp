import { on, once, printConsole, storage, settings, Game, findConsoleCommand } from 'skyrimPlatform';
import { WorldView } from './view';
import { getMovement} from './components/movement';
import { getLook } from './components/look';
import { AnimationSource, Animation, setupHooks } from './components/animation';
import { getEquipment } from './components/equipment';
import { MsgType } from './messages';
import { MsgHandler } from './msgHandler';
import { ModelSource } from './modelSource';
import { RemoteServer } from './remoteServer';
import { SendTarget } from './sendTarget';
import * as networking from './networking';
import * as sp from 'skyrimPlatform';
import * as loadGameManager from './loadGameManager';
import { WorldModel, FormModel } from './model';

let handleMessage = (msgAny: any, handler: MsgHandler) => {
    let msgType = msgAny.type || MsgType[msgAny.t];
    let f = handler[msgType];
    if (msgType !== 'UpdateMovement') printConsole(msgType, msgAny);
    if (f && typeof f === 'function') handler[msgType](msgAny);
};

for (let i = 0; i < 100; ++i) printConsole();
printConsole('Hello Multiplayer');
printConsole('settings:', settings['skymp5-client']);

let targetIp = settings['skymp5-client']['server-ip'];
let targetPort = settings['skymp5-client']['server-port'];;

if (storage.targetIp !== targetIp || storage.targetPort !== targetPort) {
    storage.targetIp = targetIp;
    storage.targetPort = targetPort;

    printConsole(`Connecting to ${storage.targetIp}:${storage.targetPort}`);
    networking.connect(targetIp, targetPort);
}
else {
    printConsole('Reconnect is not required');
}

export class SkympClient {
    constructor() {
        this.resetView();
        this.resetRemoteServer();
        setupHooks();

        networking.on('connectionFailed', () => {
            printConsole('Connection failed');
        });

        networking.on('connectionDenied', (err: string) => {
            printConsole('Connection denied: ', err);
        });

        networking.on('connectionAccepted', () => {
            this.msgHandler.handleConnectionAccepted();
        });

        networking.on('disconnect', () => {
            this.msgHandler.handleDisconnect();
        });

        networking.on('message', (msgAny: any) => {
            handleMessage(msgAny, this.msgHandler);
        });

        on('update', () => { 
            if (!this.singlePlayer) {
                this.sendInputs(); 
            }
        });

        
        const playerFormId = 0x14;
        on('equip', e => {
            if (e.actor.getFormID() === playerFormId) this.equipmentChanged = true;
        });
        on('unequip', e => {
            if (e.actor.getFormID() === playerFormId) this.equipmentChanged = true;
        });

        loadGameManager.addLoadGameListener((e: loadGameManager.GameLoadEvent) => {
            if (!e.isCausedBySkyrimPlatform && !this.singlePlayer) {
                sp.Debug.messageBox("Save has been loaded in multiplayer, switching to the single-player mode");
                networking.close();
                this.singlePlayer = true;
                Game.setInChargen(false, false, false);
            }
        });
    }    

    private sendMovement() { 
        const sendMovementRateMs = 130;   
        let now = Date.now();
        if (now - this.lastSendMovementMoment > sendMovementRateMs) {
            this.sendTarget.send({ t: MsgType.UpdateMovement, data: getMovement(Game.getPlayer()) }, false);
            this.lastSendMovementMoment = now;
        }
    }

    private sendAnimation() {
        if (!this.playerAnimSource) {
            this.playerAnimSource = new AnimationSource(Game.getPlayer());
        }
        let anim = this.playerAnimSource.getAnimation();
        if (!this.lastAnimationSent || anim.numChanges !== this.lastAnimationSent.numChanges) {
            if (anim.animEventName !== '') {
                this.lastAnimationSent = anim;
                this.sendTarget.send({ t: MsgType.UpdateAnimation, data: anim }, false);
            }
        }
    }

    private sendLook() {
        let shown = sp['UI']['isMenuOpen']('RaceSex Menu'); 
        if (shown != this.isRaceSexMenuShown) {
            this.isRaceSexMenuShown = shown;
            if (!shown) {
                printConsole('Exited from race menu');

                let look = getLook(Game.getPlayer());
                this.sendTarget.send({ t: MsgType.UpdateLook, data: look }, true);
                for (let key in look) {
                    printConsole(`${key} = ${JSON.stringify(look[key])}`);
                }
            }
        }
    }

    private sendEquipment() {
        if (this.equipmentChanged) {
            this.equipmentChanged = false;

            ++this.numEquipmentChanges;
            
            let eq = getEquipment(Game.getPlayer(), this.numEquipmentChanges);
            this.sendTarget.send({ t: MsgType.UpdateEquipment, data: eq }, true);
            printConsole({eq});
        }
    }

    private sendInputs() {
        this.sendMovement();
        this.sendAnimation();
        this.sendLook();
        this.sendEquipment();
    }

    private resetRemoteServer() {
        let prevRemoteServer: RemoteServer = storage.remoteServer;
        let rs: RemoteServer;

        if (prevRemoteServer && prevRemoteServer.getWorldModel) {
            rs = prevRemoteServer;
            printConsole('Restore previous RemoteServer');
            
            // Keep previous RemoteServer, but update func implementations
            let newObj = new RemoteServer;
            for (let key in newObj) {
                if (typeof newObj[key] === 'function') rs[key] = newObj[key];
            }
        }
        else {
            rs = new RemoteServer;
            printConsole('Creating RemoteServer');
        }

        this.sendTarget = rs;
        this.msgHandler = rs;
        this.modelSource = rs
        storage.remoteServer = rs;

        let lookTints = null;

        if (0)this.modelSource = {
            getWorldModel(): WorldModel {
                if (!lookTints && getLook(Game.getPlayer()).tints.length > 0) {
                    lookTints = getLook(Game.getPlayer());
                }
                /*let movement = getMovement(Game.getPlayer());
                let look = getLook(Game.getPlayer());
                let forms = new Array<FormModel>();
                forms.length = 3;
                forms.forEach((f, i) => { forms[i] = { look, movement } })*/
                //let randColor = 0xffffff * (Math.ceil(Date.now() / 1000) % 10) / 10;
                //printConsole(randColor);
                let forms = new Array<FormModel>();
                let imax = 10;
                let jmax = 10;
                let m = getMovement(Game.getPlayer());
                let l = getLook(Game.getPlayer());

                let colors = [0xffffffff, 0xffff0000, 0xff00ff00, 0xff0000ff, 0xff33ccff/*, 0xffccff33*/];

                for(let i = 0; i < imax; ++i) {
                    for(let j = 0; j < jmax; ++j) {
                        let randColor;

                        //randColor = 0xffffff * ((i+1 * j+1) % (jmax * imax)) / (jmax * imax);
                        randColor = colors[(j * jmax + i) % colors.length];

                        forms.push({movement: JSON.parse(JSON.stringify(m))});
                        forms[forms.length - 1].movement.pos[0] += 128 * (1 + i);
                        forms[forms.length - 1].movement.pos[1] += 128 * (1 + j);
                        forms[forms.length - 1].look = JSON.parse(JSON.stringify(l));
                        if (lookTints) forms[forms.length -1].look.tints = JSON.parse(JSON.stringify(lookTints.tints));

                        if (Math.ceil(Date.now() / 1000) % 2 == 1) {
                            ///randColor = 0xffffff * ((jmax * imax) - (i+1 * j+1) % (jmax * imax)) / (jmax * imax);
                            //forms[forms.length - 1].look.hairColor = -1;
                        }

                        
                        forms[forms.length - 1].look.tints.forEach(t => {
                            if (t.argb > 0xffffff || t.argb < 0) t.argb = randColor - 0xffffffff/2
                        });
                        //forms[forms.length - 1].look.tints.find(t => t.texturePath.match(/.*/)).argb = randColor - 0xffffffff/2;
                        forms[forms.length - 1].look.skinColor = randColor - 0xffffffff/2;
                        forms[forms.length - 1].look.hairColor = randColor - 0xffffffff/2;

                        if (!lookTints) printConsole("Sorry", getLook(Game.getPlayer()).tints.length );
                    }
                }
                return {
                    forms,
                    playerCharacterFormIdx: -1
                };
            }
        };
    }

    private resetView() {
        let prevView: WorldView = storage.view;
        let view = new WorldView;
        once('update', () => {
            if (prevView && prevView.destroy) {
                prevView.destroy();
                printConsole('Previous View destroyed');
            }
            storage.view = view;
        });
        on('update', () => {
            if (!this.singlePlayer) view.update(this.modelSource.getWorldModel());
        });
    }

    private playerAnimSource?: AnimationSource;
    private lastSendMovementMoment = 0;
    private lastAnimationSent?: Animation;
    private msgHandler?: MsgHandler;
    private modelSource?: ModelSource;
    private sendTarget?: SendTarget;
    private isRaceSexMenuShown = false;
    private singlePlayer = false;
    private equipmentChanged = false;
    private numEquipmentChanges = 0;
}

findConsoleCommand('showracemenu').execute = () => {
    printConsole('bope');
    return false;
};

// TODO: remove this
once('update', () => {
    Game.getPlayer().unequipAll();
    Game.getPlayer().addItem(Game.getFormEx(0x0001397D), 100, true);
});

let enforceLimitations = () => {
    Game.setInChargen(true, true, false);
}

once('update', enforceLimitations);
loadGameManager.addLoadGameListener(enforceLimitations);