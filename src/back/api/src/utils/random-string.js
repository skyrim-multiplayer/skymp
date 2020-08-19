const randomString = length => {
  let str = "";

  while (str.length < length) {
    str += Math.random()
      .toString(36)
      .substring(2);
  }

  return str.substring(0, length);
};

module.exports = randomString;
