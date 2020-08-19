const {
  AuthenticationError,
  UserInputError,
  ApolloError
} = require("apollo-server-express");
const bcrypt = require("bcrypt");

const { decrypt } = require("../../../utils/crypto");
const tokenUtil = require("../../../utils/create-token");
const regex = require("../../../utils/regex");
const { sendVerifySuccess } = require("../../../emails");

const User = require("../../../models/user");

const verify = async (_, { username, password, code }) => {
  try {
    if (regex.nameRegex.test(username)) {
      const user = await User.findOne({ username });
      if (!user) {
        throw new AuthenticationError();
      }

      if (await bcrypt.compare(password, user.password)) {
        const hashVerificationCode = user.auth.verificationCodes.signup;
        const isExpires = hashVerificationCode
          ? Date.now() - hashVerificationCode.expiresTime < 0
          : false;

        if (!isExpires) {
          throw new UserInputError("Code is expired");
        }

        if (
          hashVerificationCode &&
          (await bcrypt.compare(code, hashVerificationCode.value))
        ) {
          user.auth.isVerified = true;
          user.auth.verificationCodes.signup = null;

          await user.save(err => {
            if (err) {
              throw new ApolloError();
            }
          });

          const token = tokenUtil.create(user._id, user.username);

          const res = {
            user: {
              id: user._id,
              username: user.username,
              email: decrypt(user.email)
            },
            token
          };

          sendVerifySuccess({
            email: res.user.email,
            username: res.user.username
          });

          return res;
        } else {
          throw new UserInputError("Incorrect code");
        }
      } else {
        throw new AuthenticationError();
      }
    } else {
      throw new UserInputError("Incorrect login");
    }
  } catch (err) {
    throw new AuthenticationError(err);
  }
};

module.exports = verify;
