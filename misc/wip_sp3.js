// This is an incomplete test for an incomplete SP3 implementation

// Replicates the behavior of the pretify function from TSConverter
// Except it makes the first character lowercase by default
function prettify(name) {
    let func = ch => ch.toLowerCase();
    let firstChar = func(name.charAt(0));
    name = name.slice(1);

    const lowerCaseName = name.toLowerCase();
    const upperCaseName = name.toUpperCase();

    if (name === lowerCaseName || name === upperCaseName) {
        name = name.toLowerCase();
    }

    return firstChar + name;
};

function sortClassesByInheritance(classes, api) {
    let classesSorted = new Array;

    while (classes.length > 0) {
        let stack = [classes.shift()];
        let baseClass = stack[0];
        while (baseClass) {
            baseClass = api._sp3GetBaseClass(baseClass);
            if (baseClass) {
                stack.push(baseClass);
            }
        }
        stack.reverse();
        for (let i = 0; i < stack.length; i++) {
            if (!classesSorted.includes(stack[i])) {
                classesSorted.push(stack[i]);
            }
        }
        classes = classes.filter(c => !stack.includes(c));
    }

    return classesSorted;
}

// While SP3 backend is trusted, it's better to be safe than sorry
function sanitizeClassNameBeforeEval(className) {
    const alphabet = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789_-";

    const needThrowSecurityError = className.split("").some(ch => !alphabet.includes(ch));

    if (needThrowSecurityError) {
        const classNameBase64 = Buffer.from(className).toString("base64");
        throw new Error("sanitizeClassNameBeforeEval - Invalid class name (base64-encoded): " + classNameBase64);
    }
}

// for each argument if creeationTickId is set, verify it's the same as the current tick id
// if not, throw an error
function verifyTickIds(api, args, spPrivate, className, functionName) {
    const creationTickIds = args.map(arg => spPrivate.creationTickIds.get(arg)).filter(Boolean);
    const currentTickId = api._sp3GetCurrentTickId();
    if (creationTickIds.some(creationTickId => creationTickId !== currentTickId)) {
        throw new Error(`An expired object passed to ${className}.${functionName}`);
    }
}

let assign = (targetObject, sourceObject) => {
    if (sourceObject.desc !== undefined) {
        // Server-side/gamemode raw objects have desc and type properties
        targetObject.desc = sourceObject.desc;
        targetObject.type = sourceObject.type;

        // Never check it in runtime again
        assign = (targetObject, sourceObject) => {
            targetObject.desc = sourceObject.desc;
            targetObject.type = sourceObject.type;
        };

    } else if (sourceObject._skyrimPlatform_indexInPool !== undefined) {
        // Client-side objects have _skyrimPlatform_indexInPool property starting with SP3 Update in 2024
        // In previous version, they were completely native objects, without javascript properties
        targetObject._skyrimPlatform_indexInPool = sourceObject._skyrimPlatform_indexInPool;

        // Never check it in runtime again
        assign = (targetObject, sourceObject) => {
            targetObject._skyrimPlatform_indexInPool = sourceObject._skyrimPlatform_indexInPool;
        };
    }
}

function createSkyrimPlatform(api) {
    const sp = {};
    const spPrivate = {
        isCtorEnabled: false,
        creationTickIds: new WeakMap()
    };

    //console.log(api.get)

    const classes = sortClassesByInheritance(api._sp3ListClasses(), api);

    console.log({ classes })

    classes.forEach(className => {
        const baseClassName = api._sp3GetBaseClass(className);
        const staticFunctions = api._sp3ListStaticFunctions(className);
        const methods = api._sp3ListMethods(className);

        console.log({ className, baseClassName, staticFunctions, methods: JSON.stringify(methods) })

        sanitizeClassNameBeforeEval(className);

        const onConstruct = (resWithClass) => {
            if (!spPrivate.isCtorEnabled) {
                throw new Error(`Direct construction of ${className} is not allowed`);
            }

            spPrivate.creationTickIds.set(resWithClass, api._sp3GetCurrentTickId());
        };

        const f = eval(`
            (function ${className}() {
              onConstruct(this);  
            })
          `);

        if (baseClassName !== "") {
            f.prototype = Object.create(sp[baseClassName].prototype);
        }

        f.prototype.constructor = f;

        methods.concat(methods.map(prettify)).forEach(method => {
            // TODO(1):
            const impl = api._sp3GetFunctionImplementation(sp, className, method);
            f.prototype[method] = function () {
                verifyTickIds(api, Array.from(arguments), spPrivate, className, method);

                const resWithoutClass = impl.bind(this)(...arguments);
                if (resWithoutClass === null || typeof resWithoutClass !== "object") {
                    return resWithoutClass;
                }
                const _sp3ObjectType = resWithoutClass._sp3ObjectType;
                const ctor = sp[_sp3ObjectType];
                spPrivate.isCtorEnabled = true;
                const resWithClass = new ctor();
                spPrivate.isCtorEnabled = false;
                assign(resWithClass, resWithoutClass);
                return resWithClass;
            };
        });

        staticFunctions.concat(staticFunctions.map(prettify)).forEach(staticFunction => {

            const impl = api._sp3GetFunctionImplementation(sp, className, staticFunction);
            f[staticFunction] = function () {
                verifyTickIds(api, Array.from(arguments), spPrivate, className, staticFunction);
                const resWithoutClass = impl(...arguments);
                if (resWithoutClass === null || typeof resWithoutClass !== "object") {
                    return resWithoutClass;
                }
                const _sp3ObjectType = resWithoutClass._sp3ObjectType;
                const ctor = sp[_sp3ObjectType];
                spPrivate.isCtorEnabled = true;
                const resWithClass = new ctor();
                spPrivate.isCtorEnabled = false;
                assign(resWithClass, resWithoutClass);
                return resWithClass;
            };
        });

        f["from"] = function (obj) {
            verifyTickIds(api, [obj], spPrivate, className, "from");
            if (api._sp3DynamicCast(obj, className)) {
                spPrivate.isCtorEnabled = true;
                const resWithClass = new f();
                spPrivate.isCtorEnabled = false;
                assign(resWithClass, obj);
                return resWithClass;
            }
            return null;
        }

        sp[className] = f;
    });

    return sp;
}

const sp = createSkyrimPlatform(mp);

// console.log(1);

// console.log(sp.Game.getFormEx(0xff000000));
// console.log(sp.Game.getFormEx(0x7));

// console.log(sp.Utility.randomInt(1, 100));

// console.log("===", sp.Utility.randomInt(1, 1));

// sp.Skymp.setDefaultActor();
// console.log(sp.Game.getPlayer());

//var createdId = mp.createActor(0, [0,0,0], 0, 0x3c);
//console.log("createdId!", createdId);

var form = sp.Game.getFormEx(4278190082);
console.log("form!", form);
form = { desc: "0", type: "form" };
console.log("form!", form);

var actor = sp.Actor.from(form);
console.log("actor!", actor);

sp.Skymp.setDefaultActor(actor);
console.log("player!", sp.Game.GetPlayer());


var dead = [sp.Game.GetPlayer().isDead()];
console.log({ dead })

var pos = [sp.Game.GetPlayer().getPositionX()];
console.log({ pos })

sp.Game.getPlayer().setPosition(100, 200, 300);

var pos = [sp.Game.GetPlayer().getPositionX()];
console.log({ pos })

process.exit(0);
