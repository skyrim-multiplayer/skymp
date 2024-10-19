// This is an incomplete test for an incomplete SP3 implementation

// TODO(1): Implement typed return values for methods and static functions (e.g. `sp.Game.getFormEx` should return a `Form` object, not a plain object)
// TODO(2): Normalize class names to match SkyrimPlatform's naming convention (e.g. objectreference -> ObjectReference)
// TODO(3): Implement 2 versions of method names: one with the SP-style/normalized naming convention (e.g. `getFormEx`) and one with the Papyrus naming convention (e.g. `GetFormEx`)

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
  
    console.log({classes})
  
    classes.forEach(className => {
        const baseClassName = api._sp3GetBaseClass(className);
        const staticFunctions = api._sp3ListStaticFunctions(className);
        const methods = api._sp3ListMethods(className);
  
        console.log({className, baseClassName, staticFunctions, methods: JSON.stringify(methods)})
  
        const f = eval(`(function ${className}() {})`);
  
        if (baseClassName !== "") {
            f.prototype = Object.create(sp[baseClassName].prototype);
        }
  
        f.prototype.constructor = f;
  
        methods.forEach(method => {
            // TODO(1):
            // const impl = api._sp3GetFunctionImplementation(sp, className, method);
            // f.prototype[method] = function () {
            //   const resWithoutClass = impl(this, ...arguments);
            // };
  
            f.prototype[method] = api._sp3GetFunctionImplementation(sp, className, method);
        });
  
        staticFunctions.forEach(staticFunction => {
            // TODO(1):
            f[staticFunction] = api._sp3GetFunctionImplementation(sp, className, staticFunction);
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
  
  var form = sp.game.GetFormEx(4278190082);
  console.log("form!", form);
  form = { desc: "0", type:"form"};
  console.log("form!", form);
  
  var actor = sp.actor.from(form);
  console.log("actor!", actor);
  
  sp.skymp.SetDefaultActor(actor);
  console.log("player!", sp.game.GetPlayer());
  
  var pos = [sp.game.GetPlayer().GetPositionX()];
  console.log({pos})
  
  process.exit(0);
  