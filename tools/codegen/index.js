let fs = require('fs');

let prettify = (name, f = ''.toUpperCase) => {
    let c = f.apply(name.charAt(0));
    return (name.toUpperCase() === name || name.toLowerCase() === name) 
        ? c + name.slice(1).toLowerCase() 
        : c + name.slice(1);
};

const p = 'C:\\Program Files (x86)\\Steam\\steamapps\\common\\Skyrim Special Edition\\Data\\Platform\\Output\\FunctionsDump.txt';
const source = JSON.parse(fs.readFileSync(p));
const tab = '    ';
const ignored = ['TESModPlatform.Add', 'Math'];
const functionNameOverrides = {'getplayer': 'getPlayer'};

let output = `
// Generated automatically. Do not edit.
export declare function printConsole(...arguments: any[]): void;
export declare function writeScript(scriptName: string, src: string): void;
export declare function on(eventName: string, callback: any): void;
export declare function callNative(className: string, functionName: string, self?: object, ...args: any): any;
export declare function getJsMemoryUsage(): number;
export declare let storage: any;

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
    f.arguments.forEach((arg, i) => {
        output += `${arg.name}: ${parseReturnValue(arg.type)}`;
        if (i !== f.arguments.length - 1) {
            output += `, `;
        }
    });
    let returnType = parseReturnValue(f.returnType);
    if (f.isLatent) {
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

fs.writeFileSync('out.ts', output);