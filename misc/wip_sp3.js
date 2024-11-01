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

function createSkyrimPlatform(api) {

    let sp = {};

    //console.log(api.get)

    let classes = sortClassesByInheritance(api._sp3ListClasses(), api);

    console.log({ classes })

    classes.forEach(className => {
        const baseClassName = api._sp3GetBaseClass(className);
        const staticFunctions = api._sp3ListStaticFunctions(className);
        const methods = api._sp3ListMethods(className);

        console.log({ className, baseClassName, staticFunctions, methods: JSON.stringify(methods) })

        const f = eval(`(function ${className}() {})`);

        if (baseClassName !== "") {
            f.prototype = Object.create(sp[baseClassName].prototype);
        }

        f.prototype.constructor = f;

        methods.concat(methods.map(prettify)).forEach(method => {
            // TODO(1):
            const impl = api._sp3GetFunctionImplementation(sp, className, method);
            f.prototype[method] = function () {
                const resWithoutClass = impl.bind(this)(...arguments);
                if (resWithoutClass === null || typeof resWithoutClass !== "object") {
                    return resWithoutClass;
                }
                const _sp3ObjectType = resWithoutClass._sp3ObjectType;
                const ctor = sp[_sp3ObjectType];
                const implWithClass = new ctor();
                implWithClass.type = resWithoutClass.type;
                implWithClass.desc = resWithoutClass.desc;
                return implWithClass;
            };
        });

        staticFunctions.concat(staticFunctions.map(prettify)).forEach(staticFunction => {

            const impl = api._sp3GetFunctionImplementation(sp, className, staticFunction);
            f[staticFunction] = function () {
                const resWithoutClass = impl(...arguments);
                if (resWithoutClass === null || typeof resWithoutClass !== "object") {
                    return resWithoutClass;
                }
                const _sp3ObjectType = resWithoutClass._sp3ObjectType;
                const ctor = sp[_sp3ObjectType];
                const implWithClass = new ctor();
                implWithClass.type = resWithoutClass.type;
                implWithClass.desc = resWithoutClass.desc;
                return implWithClass;
            };
        });

        f["from"] = function (obj) {
            if (api._sp3DynamicCast(obj, className)) {
                let res = new f();
                Object.assign(res, obj);
                return res;
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
