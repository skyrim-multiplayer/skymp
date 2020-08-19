const {
  AuthenticationError,
  UserInputError
} = require("apollo-server-express");
const bcrypt = require("bcrypt");

const { encrypt } = require("../../../utils/crypto");
const randomString = require("../../../utils/random-string");
const regex = require("../../../utils/regex");
const { sendVerify } = require("../../../emails");

const User = require("../../../models/user");

const config = require("../../../config");

const signup = async (_, { username, email, password }) => {
  try {
    // validation:
    if (username.length < 2) {
      throw new UserInputError(
        "Incorrect username, username should be min 2 symbols length"
      );
    } else if (username.length > 32) {
      throw new UserInputError(
        "Incorrect username, username should be max 32 symbols length"
      );
    }

    if (!regex.nameRegex.test(username)) {
      throw new UserInputError(
        "Incorrect username, username must contain only Latin letters, hyphens, apostrophes."
      );
    }

    if (!regex.emailRegex.test(email)) {
      throw new UserInputError("Incorrect email");
    }

    if (password.length < 8) {
      throw new UserInputError(
        "Incorrect password, password should be min 8 symbols"
      );
    }

    if (password.length > 32) {
      throw new UserInputError(
        "Incorrect password, password should be max 32 symbols"
      );
    }

    if (await User.findOne({ username })) {
      throw new UserInputError("Username is busy");
    }

    const verificationCodeValue = randomString(config.verifyCode.length);

    const user = await User.create({
      username,
      email: encrypt(email),
      password: await bcrypt.hash(password, config.jwt.salt),
      auth: {
        verificationCodes: {
          signup: {
            value: await bcrypt.hash(verificationCodeValue, config.jwt.salt)
          }
        }
      }
    });

    sendVerify({ email, username, code: verificationCodeValue });

    return {
      id: user._id,
      username: user.username,
      email: email,
      isVerified: false
    };
  } catch {
    throw new AuthenticationError();
  }
};

module.exports = signup;
