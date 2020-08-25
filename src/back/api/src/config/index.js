const result = require("dotenv").config();
if (result.error) {
  // We don't throw error here since Docker container doesn't use dotenv at all
  console.log("dotenv failed with error:", result.error);
}

const cfg = {
  db: {
    username: process.env.AUTH_DB_USERNAME,
    password: process.env.AUTH_DB_PASSWORD,
    database: process.env.AUTH_DB_NAME,
    host: process.env.AUTH_DB_HOST,
  },
  jwt: {
    salt: 10,
    secret: process.env.AUTH_JWT_SECRET,
    lifetime: "10d",
  },
  aes: {
    algorithm: "aes256",
    key: process.env.AUTH_AES_KEY,
    iv: process.env.AUTH_AES_IV,
  },
  verifyCode: {
    length: 6,
  },
  nodemailer: {
    user: process.env.AUTH_EMAIL_USER,
    password: process.env.AUTH_EMAIL_PASSWORD,
  },
  port: 5000,
};

module.exports = cfg;
