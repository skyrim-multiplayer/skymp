const auth = require("./auth");

module.exports = {
  typeDefs: [auth.typeDef],
  schemaDirectives: {
    auth: auth.directive
  }
};
