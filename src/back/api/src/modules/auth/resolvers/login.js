const {
  AuthenticationError,
  UserInputError
} = require("apollo-server-express");
const bcrypt = require("bcrypt");

const { decrypt } = require("../../../utils/crypto");
const tokenUtil = require("../../../utils/create-token");
const regex = require("../../../utils/regex");
const User = require("../../../models/user");

const login = async (_, { username, password }) => {
  try {
    if (regex.nameRegex.test(username)) {
      const user = await User.findOne({ username });

      if (user && (await bcrypt.compare(password, user.password))) {
        if (!user.auth.isVerified) {
          throw new AuthenticationError("Not verified");
        }

        const token = tokenUtil.create(user._id, user.username);

        return {
          user: {
            id: user._id,
            username: user.username,
            email: decrypt(user.email)
          },
          token
        };
      } else {
        throw new UserInputError("Incorrect username or password");
      }
    } else {
      throw new UserInputError("Login invalid");
    }
  } catch {
    throw new AuthenticationError();
  }
};

module.exports = login;
