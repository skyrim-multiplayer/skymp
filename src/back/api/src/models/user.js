const mongoose = require("mongoose");

const verifyCodeSchema = new mongoose.Schema({
  value: {
    type: String,
    required: true,
  },
  expiresTime: {
    type: Date,
    default: () => new Date(+new Date() + 2 * 60 * 60 * 1000), // 2h
    required: true,
  },
});

const userSchema = new mongoose.Schema({
  username: {
    type: String,
    required: true,
    unique: true,
  },

  // hashed
  email: {
    type: String,
    requred: true,
    unique: true,
  },

  // hashed
  password: {
    type: String,
    required: true,
  },

  auth: {
    /*
      `true` when user will verify with email, another `false`
    */
    isVerified: {
      type: Boolean,
      required: true,
      default: false,
    },

    // TODO: Do it automatically
    /* for auto delete you should create collection index:
     *  db.users.createIndex(
          { 'auth.verificationExpires': 1 },
          {
            expireAfterSeconds: 0,
            partialFilterExpression: { 'auth.isVerified': false }
          }
        )
     */
    verificationExpires: {
      type: Date,
      default: () => new Date(+new Date() + 2 * 60 * 60 * 1000), // 2h
      required: true,
    },
    /**
     * send to email for verification
     */
    verificationCodes: {
      signup: verifyCodeSchema,
      recovery: verifyCodeSchema,
    },
  },

  createdAt: { type: Date, default: Date.now },
});

module.exports = mongoose.model("User", userSchema, "users");
