const { AuthenticationError } = require("apollo-server-express");

const me = async (_, args, { user }) => {
  if (!user) {
    throw new AuthenticationError("You are not authenticated");
  } else
    return {
      id: user.id,
      username: user.username,
      email: user.email
    };
};

module.exports = me;
