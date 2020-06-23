const env = process.env.NODE_ENV || "development";

const config = env === "development" ? "Debug" : "Release";
console.log(`Using scamp_native config ${config}`);

const scampNative = require(`../build/${config}/scamp_native.node`);
console.log(scampNative.hello());

(async () => {
  while (1) {
    await new Promise(setImmediate);
  }
})();
