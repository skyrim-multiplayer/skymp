const result = require("dotenv").config();
if (result.error) {
  throw result.error;
}

module.exports = {
  db: {
    username: process.env.DB_USERNAME,
    password: process.env.DB_PASSWORD,
    database: process.env.DB_NAME,
    host: process.env.DB_HOST
  },
  jwt: {
    salt: +process.env.JWT_SALT,
    secret: process.env.JWT_SECRET,
    lifetime: process.env.JWT_LIFE_TIME
  },
  aes: {
    algorithm: process.env.AES_ALGORITHM,
    key: process.env.AES_KEY,
    iv: process.env.AES_IV
  },
  verifyCode: {
    length: +process.env.VERIFY_CODE_LENGTH
  },
  nodemailer: {
    user: process.env.EMAIL_USER,
    password: process.env.EMAIL_PASSWORD
  },
  port: process.env.PORT
};
