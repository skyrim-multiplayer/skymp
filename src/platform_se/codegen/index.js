let fs = require('fs');
let path = require('path');

let prettify = (name, f = ''.toUpperCase) => {
    let c = f.apply(name.charAt(0));
    return (name.toUpperCase() === name || name.toLowerCase() === name)
        ? c + name.slice(1).toLowerCase()
        : c + name.slice(1);
};

const p = path.resolve(__dirname, 'FunctionsDump.txt');
const source = JSON.parse(fs.readFileSync(p));
const tab = '    ';
const ignored = ['TESModPlatform.Add', 'Math', 'MpClientPlugin' /*obsolete*/];
const functionNameOverrides = {'getplayer': 'getPlayer'};

let output = `
// Generated automatically. Do not edit.
export declare function printConsole(...arguments: any[]): void;
export declare function writeScript(scriptName: string, src: string): void;
export declare function callNative(className: string, functionName: string, self?: object, ...args: any): any;
export declare function getJsMemoryUsage(): number;
export declare let storage: any;
export declare let settings: any;

export declare function on(eventName: 'update', callback: () => void): void;
export declare function once(eventName: 'update', callback: () => void): void;

export declare function on(eventName: 'tick', callback: () => void): void;
export declare function once(eventName: 'tick', callback: () => void): void;

export declare function loadGame(pos: number[], angle: number[], worldOrCell: number);

export declare function worldPointToScreenPoint(...args: number[][]): number[][];

export type PacketType = 'message' | 'disconnect' | 'connectionAccepted' | 'connectionFailed' | 'connectionDenied';

// Available only if multiplayer is installed on user's machine
interface MpClientPlugin {
    getVersion(): string;
    createClient(host: string, port: number): void;
    destroyClient(): void;
    isConnected(): boolean;
    tick(tickHandler: (packetType: PacketType, jsonContent: string, error: string) => void): void;
    send(jsonContent: string, reliable: boolean): void;
}
export declare let mpClientPlugin: MpClientPlugin;

export interface ActivateEvent {
    target: ObjectReference,
    caster: ObjectReference,
    isCrimeToActivate: boolean
}

export interface MoveAttachDetachEvent {
    movedRef: ObjectReference,
    isCellAttached: boolean
}
export interface WaitStopEvent {
    isInterrupted: boolean
}
export interface ObjectLoadedEvent {
    object: Form,
    isLoaded: boolean
}
export interface LockChangedEvent {
    lockedObject: ObjectReference
}

export interface CellFullyLoadedEvent {
    cell: Cell
}

export interface GrabReleaseEvent {
    refr: ObjectReference,
    isGrabbed: boolean
}

export interface SwitchRaceCompleteEvent {
    subject: ObjectReference
}

export interface UniqueIDChangeEvent {
    oldBaseID: number,
    newBaseID: number,
    oldUniqueID: number,
    newUniqueID: number
}

export interface TrackedStatsEvent {
    statName: string,
    newValue: number
}

export interface InitScriptEvent {
    initializedObject: ObjectReference
}

export interface ResetEvent {
    object: ObjectReference
}

export interface CombatEvent {
    target: ObjectReference,
    actor: ObjectReference,
    isCombat: boolean,
    isSearching: boolean
}

export interface DeathEvent {
    actorDying: ObjectReference,
    actorKiller: ObjectReference
}

export interface ContainerChangedEvent {
    oldContainer: ObjectReference,
    newContainer: ObjectReference,
    baseObj: Form,
    numItems: number,
    uniqueID: number,
    reference: ObjectReference
}

export interface HitEvent {
    target: ObjectReference,
    agressor: ObjectReference,
    source: Form,
    projectile: Projectile,
    isPowerAttack: boolean,
    isSneakAttack: boolean,
    isBashAttack: boolean,
    isHitBlocked: boolean
}

export interface EquipEvent {
    actor: ObjectReference,
    baseObj: Form,
    uniqueId: number,
    originalRefr: ObjectReference
}

export interface ActiveEffectApplyRemoveEvent {
    effect: MagicEffect,
    caster: ObjectReference,
    target: ObjectReference
}

export interface MagicEffectApplyEvent {
    effect: MagicEffect,
    caster: ObjectReference,
    target: ObjectReference
}

export declare function on(eventName: 'activate', callback: (event: ActivateEvent) => void): void;
export declare function once(eventName: 'activate', callback: (event: ActivateEvent) => void): void;

export declare function on(eventName: 'waitStop', callback: (event: WaitStopEvent) => void): void;
export declare function once(eventName: 'waitStop', callback: (event: WaitStopEvent) => void): void;

export declare function on(eventName: 'objectLoaded', callback: (event: ObjectLoadedEvent) => void): void;
export declare function once(eventName: 'objectLoaded', callback: (event: ObjectLoadedEvent) => void): void;

export declare function on(eventName: 'moveAttachDetach', callback: (event: MoveAttachDetachEvent) => void): void;
export declare function once(eventName: 'moveAttachDetach', callback: (event: MoveAttachDetachEvent) => void): void;

export declare function on(eventName: 'lockChanged', callback: (event: LockChangedEvent) => void): void;
export declare function once(eventName: 'lockChanged', callback: (event: LockChangedEvent) => void): void;

export declare function on(eventName: 'grabRelease', callback: (event: GrabReleaseEvent) => void): void;
export declare function once(eventName: 'grabRelease', callback: (event: GrabReleaseEvent) => void): void;

export declare function on(eventName: 'cellFullyLoaded', callback: (event: CellFullyLoadedEvent) => void): void;
export declare function once(eventName: 'cellFullyLoaded', callback: (event: CellFullyLoadedEvent) => void): void;

export declare function on(eventName: 'switchRaceComplete', callback: (event: SwitchRaceCompleteEvent) => void): void;
export declare function once(eventName: 'switchRaceComplete', callback: (event: SwitchRaceCompleteEvent) => void): void;

export declare function on(eventName: 'uniqueIdChange', callback: (event: UniqueIDChangeEvent) => void): void;
export declare function once(eventName: 'uniqueIdChange', callback: (event: UniqueIDChangeEvent) => void): void;

export declare function on(eventName: 'trackedStats', callback: (event: TrackedStatsEvent) => void): void;
export declare function once(eventName: 'trackedStats', callback: (event: TrackedStatsEvent) => void): void;

export declare function on(eventName: 'scriptInit', callback: (event: InitScriptEvent) => void): void;
export declare function once(eventName: 'scriptInit', callback: (event: InitScriptEvent) => void): void;

export declare function on(eventName: 'reset', callback: (event: ResetEvent) => void): void;
export declare function once(eventName: 'reset', callback: (event: ResetEvent) => void): void;

export declare function on(eventName: 'combatState', callback: (event: CombatEvent) => void): void;
export declare function once(eventName: 'combatState', callback: (event: CombatEvent) => void): void;

export declare function on(eventName: 'loadGame', callback: () => void): void;
export declare function once(eventName: 'loadGame', callback: () => void): void;

export declare function on(eventName: 'deathEnd', callback: (event: DeathEvent) => void): void;
export declare function once(eventName: 'deathEnd', callback: (event: DeathEvent) => void): void;

export declare function on(eventName: 'deathStart', callback: (event: DeathEvent) => void): void;
export declare function once(eventName: 'deathStart', callback: (event: DeathEvent) => void): void;

export declare function on(eventName: 'containerChanged', callback: (event: ContainerChangedEvent) => void): void;
export declare function once(eventName: 'containerChanged', callback: (event: ContainerChangedEvent) => void): void;

export declare function on(eventName: 'hit', callback: (event: HitEvent) => void): void;
export declare function once(eventName: 'hit', callback: (event: HitEvent) => void): void;

export declare function on(eventName: 'unequip', callback: (event: EquipEvent) => void): void;
export declare function once(eventName: 'unequip', callback: (event: EquipEvent) => void): void;

export declare function on(eventName: 'equip', callback: (event: EquipEvent) => void): void;
export declare function once(eventName: 'equip', callback: (event: EquipEvent) => void): void;

export declare function on(eventName: 'magicEffectApply', callback: (event: MagicEffectApplyEvent) => void): void;
export declare function once(eventName: 'magicEffectApply', callback: (event: MagicEffectApplyEvent) => void): void;

export declare function on(eventName: 'effectFinish', callback: (event: ActiveEffectApplyRemoveEvent) => void): void;
export declare function once(eventName: 'effectFinish', callback: (event: ActiveEffectApplyRemoveEvent) => void): void;

export declare function on(eventName: 'effectStart', callback: (event: ActiveEffectApplyRemoveEvent) => void): void;
export declare function once(eventName: 'effectStart', callback: (event: ActiveEffectApplyRemoveEvent) => void): void;

declare class ConsoleComand {
    longName: string;
    shortName: string;
    numArgs: number;
    execute: (...arguments: any[]) => boolean;
}
export declare function findConsoleCommand(cmdName: string): ConsoleComand;

export enum MotionType {
    Dynamic = 1,
    SphereInertia = 2,
    BoxInertia = 3,
    Keyframed = 4,
    Fixed = 5,
    ThinBoxInertia = 6,
    Character = 7
};

export declare namespace SendAnimationEventHook {
    class Context {
        selfId: number;
        animEventName: string;

        storage: Map<string, any>;
    }

    class LeaveContext extends Context {
        animationSucceeded: boolean;
    }

    class Handler {
        enter(ctx: Context);
        leave(ctx: LeaveContext);
    }

    class Target {
        add(handler: Handler)
    }
}
export declare class Hooks {
    sendAnimationEvent: SendAnimationEventHook.Target;
}

export declare let hooks: Hooks;

export declare class HttpResponse {
    body: string;
}

export declare class HttpClient {
    constructor(host: string, port?: number);
    get(path: string): Promise<HttpResponse>;
}

`;
let dumped = [];

let parseReturnValue = (v) => {
    switch (v.rawType) {
        case 'Int':
        case 'Float':
            return 'number';
        case 'Bool':
            return 'boolean';
        case 'String':
            return 'string';
        case 'IntArray':
        case 'FloatArray':
            return 'number[]';
        case 'BoolArray':
            return 'boolean[]';
        case 'StringArray':
            return 'string[]';
        case 'None':
            return 'void';
        case 'Object':
            return prettify(source.types[v.objectTypeName] ? v.objectTypeName : 'Form');
        case 'ObjectArray':
            return 'object[]';
    }
    throw new Error(`Unknown type ${v.rawType}`);
};

let dumpFunction = (className, f, isGlobal) => {
    if (ignored.includes(className + '.' + f.name)) {
        return;
    }

    let funcName = functionNameOverrides[f.name] || f.name;
    output += tab + `${isGlobal ? 'static ' : ''}${prettify(funcName, ''.toLowerCase)}`;
    output += `(`;
    let isAddOrRemove = (funcName.toLowerCase() === "additem" || funcName.toLowerCase() === "removeitem");

    f.arguments.forEach((arg, i) => {

        let isSetMotioTypeFistArg = funcName.toLowerCase() === "setmotiontype" && i === 0;
        let argType = isSetMotioTypeFistArg ? "MotionType" : parseReturnValue(arg.type);

        output += `${arg.name}: ${argType}`;
        if (i !== f.arguments.length - 1) {
            output += `, `;
        }
    });
    let returnType = parseReturnValue(f.returnType);
    if (f.isLatent) {
        if(!isAddOrRemove)
            returnType = `Promise<${returnType}>`;
    }

    output += `)`;
    output += `: ${returnType}`;
    output += `;\n`;
};

let dumpType = (data) => {
    if (ignored.includes(data.name) || dumped.includes(data.name)) {
        return;
    }

    if (data.parent) {
        dumpType(data.parent);
    }

    output += `\n// Based on ${prettify(data.name)}.pex\n`;

    output += data.parent
        ? `export declare class ${prettify(data.name)} extends ${prettify(data.parent.name)} {\n`
        : `export declare class ${prettify(data.name)} {\n`;

    output += tab + `static from(form: Form): ${prettify(data.name)};\n`;

    data.memberFunctions.forEach(f => dumpFunction(data.name, f, false));
    data.globalFunctions.forEach(f => dumpFunction(data.name, f, true));

    output += '}\n';

    dumped.push(data.name);
};

if (!source.types.WorldSpace) {
    source.types.WorldSpace = {
        parent: 'Form',
        globalFunctions: [],
        memberFunctions: []
    };
}

for (typeName in source.types) {
    let data = source.types[typeName];
    if (data.parent) {
        data.parent = source.types[data.parent];
    }
    data.name = typeName;
}

for (typeName in source.types) {
    let data = source.types[typeName];
    dumpType(data);
}

fs.writeFileSync('skyrimPlatform.ts', output);
