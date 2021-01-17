module.exports = () => {
  const typeCheck = {};

  typeCheck.number = (name, value) => {
    if (typeof value !== typeof 0) {
      throw new TypeError(
        `Expected '${name}' to be a number, but got ${JSON.stringify(
          value
        )} (${typeof value})`
      );
    }
  };

  typeCheck.avModifier = (name, value) => {
    const modifiers = ["base", "permanent", "temporary", "damage"];
    if (!modifiers.includes(value)) {
      throw new TypeError(
        `Expected '${name}' to be a modifier, but got ${JSON.stringify(
          value
        )} (${typeof value})`
      );
    }
  };

  return typeCheck;
};
