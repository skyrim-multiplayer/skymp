(api, sp) => {
  function getAliasNameImpl(originalName, envName, character) {
    const nodeProcess = require("node:process");
    if (!nodeProcess.env[envName]) {
      return null;
    }

    const aliases = nodeProcess.env[envName].split(",");

    const indexInAliases = aliases.findIndex(alias => alias.toLowerCase() === originalName.toLowerCase());

    if (indexInAliases === -1) {
      return null;
    }

    return character + indexInAliases;
  }

  function getFunctionAliasName(functionName) {
    return getAliasNameImpl(functionName, "SP3_FUNCTION_ALIASES", "y");
  }

  function getClassAliasName(className) {
    return getAliasNameImpl(className, "SP3_CLASS_ALIASES", "x");
  }


  // function getAliasesForMethods(methods) {
  //   const nodeProcess = require("node:process");
  //   if (nodeProcess.env.SP3_HAS_ALIASES === "true") {
  //     const aliases = nodeProcess.env.SP3_ALIASES.split(",");

  //     const aliasesLowerCase = aliases.map(alias => alias.toLowerCase());
  //     const methodsLowerCase = methods.map(method => method.toLowerCase());

  //     const resultAliases = [];
  //     methodsLowerCase.forEach(method => {
  //       const indexInAliases = aliasesLowerCase.findIndex(alias => alias === method);
  //       const aliasName = 'y' + indexInAliases;
  //     });
  //   }
  //   return [];
  // }

  // function getAliasesForStatics(statics) {
  //   return [];
  // }

  function prettifyImpl(name, func) {
    let firstChar = func(name.charAt(0));
    name = name.slice(1);

    const lowerCaseName = name.toLowerCase();
    const upperCaseName = name.toUpperCase();

    if (name === lowerCaseName || name === upperCaseName) {
      name = name.toLowerCase();
    }

    return firstChar + name;
  };

  // Replicates the behavior of the prettify function from TSConverter
  // Except it makes the first character lowercase by default
  function prettify(name) {
    return prettifyImpl(name, ch => ch.toLowerCase());
  };

  function prettifyUpperCase(name) {
    return prettifyImpl(name, ch => ch.toUpperCase());
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

  function createSkyrimPlatform(api, sp) {
    // TODO: de-duplicate other wrap code from this file
    api._sp3RegisterWrapObjectFunction((obj) => {
      const _sp3ObjectType = obj._sp3ObjectType;
      const ctor = sp[_sp3ObjectType];
      if (!ctor) {
        skyrimPlatform.printConsole("!!WRAPPER!!", _sp3ObjectType, JSON.stringify(obj));
      }
      spPrivate.isCtorEnabled = true;
      const resWithClass = new ctor();
      spPrivate.isCtorEnabled = false;
      assign(resWithClass, obj);
      return resWithClass;
    });

    if (!sp) {
      sp = {};
    }

    const spPrivate = {
      isCtorEnabled: false,
      creationTickIds: new WeakMap()
    };

    const classes =
      sortClassesByInheritance(api._sp3ListClasses(), api)
        .map(className => prettifyUpperCase(className));

    console.log({ classes })

    classes.forEach(className => {
      const baseClassName = prettifyUpperCase(api._sp3GetBaseClass(className));
      let staticFunctions = api._sp3ListStaticFunctions(className);
      const methods = api._sp3ListMethods(className);

      const nStaticsBefore = staticFunctions.length;
      staticFunctions = staticFunctions.filter(f => f.toLowerCase() !== "getplayer");
      const nStaticsAfter = staticFunctions.length;

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
        const impl = api._sp3GetFunctionImplementation(sp, className, method);

        const methodFinal = function () {
          verifyTickIds(api, [this], spPrivate, className, method);
          verifyTickIds(api, Array.from(arguments), spPrivate, className, method);

          let f = (resWithoutClass) => {
            if (resWithoutClass === null || typeof resWithoutClass !== "object") {
              return resWithoutClass;
            }
            const _sp3ObjectType = resWithoutClass._sp3ObjectType;
            const ctor = sp[_sp3ObjectType];
            if (!ctor) {
              skyrimPlatform.printConsole("!!METHOD!" + className + "." + method, _sp3ObjectType, JSON.stringify(resWithoutClass));
            }
            spPrivate.isCtorEnabled = true;
            const resWithClass = new ctor();
            spPrivate.isCtorEnabled = false;
            assign(resWithClass, resWithoutClass);
            return resWithClass;
          };

          let resWithoutClass = impl.bind(this)(...arguments);

          if (resWithoutClass instanceof Promise) {
            let tmp = resWithoutClass._sp3ObjectType;

            return new Promise(resolve => {
              resWithoutClass.then((res) => {
                if (res !== null && typeof res === "object" && tmp) {
                  res._sp3ObjectType = tmp;
                }
                resolve(f(res));
              });
            });
          }

          return f(resWithoutClass);
        };

        f.prototype[method] = methodFinal;
        f.prototype[getFunctionAliasName(method) || method] = methodFinal;
      });

      staticFunctions
        .concat(staticFunctions.map(prettify))
        .concat(nStaticsBefore !== nStaticsAfter ? ["getPlayer", "GetPlayer"] : [])
        .forEach(staticFunction => {

          const impl = api._sp3GetFunctionImplementation(sp, className, staticFunction);

          const staticFunctionFinal = function () {
            verifyTickIds(api, Array.from(arguments), spPrivate, className, staticFunction);

            let f = (resWithoutClass) => {
              if (resWithoutClass === null || typeof resWithoutClass !== "object") {
                return resWithoutClass;
              }
              const _sp3ObjectType = resWithoutClass._sp3ObjectType;
              const ctor = sp[_sp3ObjectType];
              if (!ctor) {
                skyrimPlatform.printConsole("!!STATIC!" + className + "." + staticFunction, _sp3ObjectType, JSON.stringify(resWithoutClass));
              }
              spPrivate.isCtorEnabled = true;
              const resWithClass = new ctor();
              spPrivate.isCtorEnabled = false;
              assign(resWithClass, resWithoutClass);
              return resWithClass;
            };

            let resWithoutClass = impl(...arguments);

            if (resWithoutClass instanceof Promise) {
              let tmp = resWithoutClass._sp3ObjectType;

              return new Promise(resolve => {
                resWithoutClass.then((res) => {
                  if (res !== null && typeof res === "object" && tmp) {
                    res._sp3ObjectType = tmp;
                  }
                  resolve(f(res));
                });
              });
            }

            return f(resWithoutClass);
          };

          f[staticFunction] = staticFunctionFinal;
          f[getFunctionAliasName(staticFunction) || staticFunction] = staticFunctionFinal;
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
      sp[getClassAliasName(className) || className] = f;
    });

    if (sp["Spell"]) {
      sp["SPELL"] = sp["Spell"];
    }
    if (sp["Weapon"]) {
      sp["WEAPON"] = sp["Weapon"];
    }

    return sp;
  }

  return createSkyrimPlatform(api, sp);
};
