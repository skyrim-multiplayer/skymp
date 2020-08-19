const { ApolloError, UserInputError } = require("apollo-server-express");
const bcrypt = require("bcrypt");

const regex = require("../../../utils/regex");
const { encrypt } = require("../../../utils/crypto");
const randomString = require("../../../utils/random-string");
const { sendRecovery } = require("../../../emails");

const User = require("../../../models/user");

const config = require("../../../config");

const recovery = async (_, { email }) => {
  try {
    if (!regex.emailRegex.test(email)) {
      throw new UserInputError("Incorrect email");
    }

    const user = await User.findOne({
      email: encrypt(email)
    });

    if (user) {
      const verificationCodeValue = randomString(config.verifyCode.length);

      user.auth.verificationCodes.recovery = {
        value: await bcrypt.hash(verificationCodeValue, config.jwt.salt)
      };

      await user.save();

      sendRecovery({
        email,
        username: user.username,
        code: verificationCodeValue
      });
    }
  } catch {
    throw new ApolloError();
  }
};

module.exports = recovery;
