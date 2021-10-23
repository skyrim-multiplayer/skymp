// SystemJS is ES6 module loader
// https://github.com/systemjs/systemjs
// Actually it's part of Skyrim Platform but this file must be passed to tsc earlier than others.
// "___systemPolyfill.js" is also hardcoded into Skyrim Platform to be loaded before "index.js"
var Context = /** @class */ (function () {
    function Context() {
    }
    return Context;
}());
var RegistrationResult = /** @class */ (function () {
    function RegistrationResult() {
    }
    return RegistrationResult;
}());
var exports = {};
var getParentDir = function (modulePath) { return modulePath.split('/').slice(0, -1).join('/'); };
var normalizePath = function (p) { return p.slice(0, 2) === './' ? p.slice(2) : p; };
var modulePath = './index';
var fixPath = function (path) {
    var newPath = path;
    while (newPath.includes('/..')) {
        var i = newPath.indexOf('/..');
        var j = i - 1;
        while (newPath.charAt(j) != '/')
            --j;
        newPath = newPath.slice(0, j) + newPath.slice(i + 3);
    }
    return newPath;
};
var registerAnonymousImpl = function (dependenciesPaths, onRegister) {
    var parentDir = getParentDir(normalizePath(modulePath));
    var exports = {};
    var res = onRegister(function (name, value) { return exports[name] = value; }, new Context);
    res.setters.forEach(function (setter, i) {
        var pathRelative = normalizePath(dependenciesPaths[i]);
        modulePath = parentDir + '/' + pathRelative;
        modulePath = fixPath(modulePath);
        setter(require(modulePath));
    });
    res.execute();
    return exports;
};
var namedRegistrations = {};
var registerNamedImpl = function (modulePath, dependenciesPaths, onRegister) {
    var exports = {};
    var res = onRegister(function (name, value) { return exports[name] = value; }, new Context);
    res.setters.forEach(function (setter, i) {
        var p = dependenciesPaths[i];
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
    register: function () {
        var args = [];
        for (var _i = 0; _i < arguments.length; _i++) {
            args[_i] = arguments[_i];
        }
        if (!args[2]) {
            return registerAnonymousImpl(args[0], args[1]);
        }
        return registerNamedImpl(args[0], args[1], args[2]);
    }
};
