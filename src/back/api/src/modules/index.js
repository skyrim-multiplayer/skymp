const { makeExecutableSchemaFromModules } = require("../utils/modules");

const auth = require("./auth");

module.exports = makeExecutableSchemaFromModules({
  modules: [auth]
});
