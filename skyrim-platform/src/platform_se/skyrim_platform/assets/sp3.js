(api, sp) => {
  const nodeProcess = require("node:process");

  function getAliasNameImpl(originalName, envName, character) {
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
    const sorted = [];
    const visited = new Set();
    const recursionStack = new Set();

    for (const startClass of classes) {
      if (visited.has(startClass)) {
        continue;
      }

      const stack = [startClass];
      recursionStack.add(startClass);

      while (stack.length > 0) {
        const current = stack[stack.length - 1];
        const base = api._sp3GetBaseClass(current);

        if (base && !visited.has(base)) {
          if (recursionStack.has(base)) {
            throw new Error(`Circular dependency detected: ${current} -> ${base}`);
          }
          stack.push(base);
          recursionStack.add(base);
        } else {
          stack.pop();
          recursionStack.delete(current);
          if (!visited.has(current)) {
            visited.add(current);
            sorted.push(current);
          }
        }
      }
    }

    return sorted;
  }

  // Server-side/gamemode raw objects have desc and type properties
  // Client-side objects have _skyrimPlatform_indexInPool property starting with SP3 Update in 2024
  // In previous version, they were completely native objects, without javascript properties
  const assign = (targetObject, sourceObject) => {
    if (sourceObject._skyrimPlatform_indexInPool !== undefined) {
      targetObject._skyrimPlatform_indexInPool = sourceObject._skyrimPlatform_indexInPool;
    } else if (sourceObject.desc !== undefined) {
      targetObject.desc = sourceObject.desc;
      targetObject.type = sourceObject.type;
    }
  }

  function createSkyrimPlatform(api, sp) {
    if (!sp) {
      sp = {};
    }

    // Helper: Wraps a raw object with its class prototype
    const wrapObject = (obj) => {
      if (obj === null || typeof obj !== "object") {
        return obj;
      }
      const ctor = sp[obj._sp3ObjectType];
      const resWithClass = Object.create(ctor.prototype);
      assign(resWithClass, obj);
      return resWithClass;
    };

    // Helper: Generic Function Handler (handles both static and instance)
    const createWrapperFunction = (impl, isStatic) => {
      return function (...args) {
        // For instance methods, bind 'this'. For static, call directly.
        const resWithoutClass = isStatic ? impl(...args) : impl.bind(this)(...args);

        if (resWithoutClass instanceof Promise) {
          // Preserve metadata potentially attached to the Promise object itself
          let tmp = resWithoutClass._sp3ObjectType;

          return new Promise((resolve, reject) => {
            resWithoutClass.then((res) => {
              if (res !== null && typeof res === "object" && tmp) {
                res._sp3ObjectType = tmp;
              }
              resolve(wrapObject(res));
            }).catch(reject);
          });
        }

        return wrapObject(resWithoutClass);
      };
    };

    api._sp3RegisterWrapObjectFunction(wrapObject);

    const classes =
      sortClassesByInheritance(api._sp3ListClasses(), api)
        .map(className => prettifyUpperCase(className));

    classes.forEach(className => {
      const baseClassName = prettifyUpperCase(api._sp3GetBaseClass(className));
      let staticFunctions = api._sp3ListStaticFunctions(className);
      const methods = api._sp3ListMethods(className);

      const nStaticsBefore = staticFunctions.length;
      staticFunctions = staticFunctions.filter(f => f.toLowerCase() !== "getplayer");
      const nStaticsAfter = staticFunctions.length;

      // The "Computed Property Name" trick is used to create a named function dynamically
      const f = {
        [className]: function () {
          throw new Error(`Direct construction of ${className} is not allowed. Use .from() or static methods.`);
        }
      }[className];

      if (baseClassName !== "") {
        f.prototype = Object.create(sp[baseClassName].prototype);
      }

      f.prototype.constructor = f;

      // Register Instance Methods
      methods.concat(methods.map(prettify)).forEach(method => {
        const impl = api._sp3GetFunctionImplementation(sp, className, method);
        const methodFinal = createWrapperFunction(impl, false);

        f.prototype[method] = methodFinal;
        f.prototype[getFunctionAliasName(method) || method] = methodFinal;
      });

      // Register Static Functions
      staticFunctions
        .concat(staticFunctions.map(prettify))
        .concat(nStaticsBefore !== nStaticsAfter ? ["getPlayer", "GetPlayer"] : [])
        .forEach(staticFunction => {
          const impl = api._sp3GetFunctionImplementation(sp, className, staticFunction);
          const staticFunctionFinal = createWrapperFunction(impl, true);

          f[staticFunction] = staticFunctionFinal;
          f[getFunctionAliasName(staticFunction) || staticFunction] = staticFunctionFinal;
        });

      f["from"] = function (obj) {
        if (api._sp3DynamicCast(obj, className)) {
          const resWithClass = Object.create(f.prototype);
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
