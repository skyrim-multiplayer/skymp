const jsonwebtoken = require("jsonwebtoken");

const config = require("../config");

const create = (userId, username) =>
  new Promise((resolve, reject) => {
    jsonwebtoken.sign(
      {
        id: userId,
        username
      },
      config.jwt.secret,
      {
        expiresIn: config.jwt.lifetime
      },
      (err, token) => {
        if (err) {
          return reject(err);
        }

        resolve(token);
      }
    );
  });

module.exports = {
  create
};
