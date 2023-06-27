// SystemJS is ES6 module loader
// https://github.com/systemjs/systemjs

// Actually it's part of Skyrim Platform but this file must be passed to tsc earlier than others.
// "___systemPolyfill.js" is also hardcoded into Skyrim Platform to be loaded before "index.js"

class Context {
  id: string | undefined;
}
class RegistrationResult {
  setters: Setter[];
  execute: () => void;
}

declare let System: Object;
declare var require: (name: string) => object;
declare let addNativeExports: (name: string, existingExports: object) => object;
declare let log: (...args: any[]) => void;
type ExportFn = (name: string, value: any) => void;
type ReigsterCallback = (
  exportFn: ExportFn,
  context: Context,
) => RegistrationResult;
type Setter = (dependencyExports: Object) => void;

let exports = {};
let getParentDir = (modulePath: string) =>
  modulePath.split('/').slice(0, -1).join('/');
let normalizePath = (p: string) => (p.slice(0, 2) === './' ? p.slice(2) : p);

let modulePath = './index';

let fixPath = (path: string) => {
  let newPath = path;
  while (newPath.includes('/..')) {
    let i = newPath.indexOf('/..');
    let j = i - 1;
    while (newPath.charAt(j) != '/') --j;
    newPath = newPath.slice(0, j) + newPath.slice(i + 3);
  }
  return newPath;
};

let registerAnonymousImpl = (dependenciesPaths, onRegister) => {
  let parentDir = getParentDir(normalizePath(modulePath));
  let exports = {};
  let res = onRegister((name, value) => (exports[name] = value), new Context());
  res.setters.forEach((setter, i) => {
    let pathRelative = normalizePath(dependenciesPaths[i]);
    modulePath = parentDir + '/' + pathRelative;
    modulePath = fixPath(modulePath);
    setter(require(modulePath));
  });
  res.execute();
  return exports;
};

let namedRegistrations = {};

let registerNamedImpl = (modulePath, dependenciesPaths, onRegister) => {
  let exports = {};
  let res = onRegister((name, value) => (exports[name] = value), new Context());
  res.setters.forEach((setter, i) => {
    let p = dependenciesPaths[i];
    if (!namedRegistrations[p]) {
      namedRegistrations[p] = addNativeExports(normalizePath(p), {});
    }
    setter(namedRegistrations[p]);
  });
  res.execute();
  exports = addNativeExports(modulePath, exports);
  namedRegistrations[modulePath] = exports;
  return exports;
};

System = {
  register: (...args) => {
    if (!args[2]) {
      return registerAnonymousImpl(args[0], args[1]);
    }
    return registerNamedImpl(args[0], args[1], args[2]);
  },
};
