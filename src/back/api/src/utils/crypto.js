const crypto = require("crypto");

const config = require("../config");

const encrypt = value => {
  const cipher = crypto.createCipheriv(
    config.aes.algorithm,
    config.aes.key,
    config.aes.iv
  );
  return Buffer.concat([cipher.update(value), cipher.final()]).toString("hex");
};

const decrypt = value => {
  const decipher = crypto.createDecipheriv(
    config.aes.algorithm,
    config.aes.key,
    config.aes.iv
  );

  const decrypted = Buffer.concat([
    decipher.update(Buffer.from(value, "hex")),
    decipher.final()
  ]);

  return decrypted.toString();
};

module.exports = {
  encrypt,
  decrypt
};
