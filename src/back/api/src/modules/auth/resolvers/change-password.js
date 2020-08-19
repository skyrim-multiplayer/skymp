const { ApolloError, UserInputError } = require("apollo-server-express");
const bcrypt = require("bcrypt");

const { decrypt } = require("../../../utils/crypto");
const tokenUtil = require("../../../utils/create-token");
const { sendRecoverySuccess } = require("../../../emails");

const User = require("../../../models/user");

const config = require("../../../config");

const changePassword = async (_, { username, newPassword, code }) => {
  try {
    if (newPassword < 8) {
      throw new UserInputError(
        "Incorrect password, password should be min 8 symbols"
      );
    }

    const user = await User.findOne({ username });

    if (user) {
      const recoveryCode = user.auth.verificationCodes.recovery;

      if (recoveryCode || Date.now() - recoveryCode.expiresTime < 0) {
        if (await bcrypt.compare(code, recoveryCode.value)) {
          user.password = await bcrypt.hash(newPassword, config.jwt.salt);
          user.auth.verificationCodes.recovery = null;

          await user.save(err => {
            if (err) {
              throw new ApolloError();
            }
          });

          const email = decrypt(user.email);

          sendRecoverySuccess({ email, username: user.username });

          const token = tokenUtil.create(user._id, user.username);

          return {
            user: {
              id: user._id,
              username: user.username,
              email
            },
            token
          };
        }
      }
    }
  } catch {
    throw new ApolloError();
  }
};

module.exports = changePassword;
