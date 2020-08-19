const me = require("./me");
const login = require("./login");
const signup = require("./signup");
const verify = require("./verify");
const recovery = require("./recovery");
const changePassword = require("./change-password");

const resolvers = {
  Query: {
    me
  },
  Mutation: {
    login,
    signup,
    verify,
    recovery,
    changePassword
  }
};

module.exports = resolvers;
