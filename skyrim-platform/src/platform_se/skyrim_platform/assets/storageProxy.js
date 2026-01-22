// Compat with pre-SP3 where storage is a Proxy, like Actor/ObjectReference/etc.
// The user code, including skymp5-client and skymp5-gamemode, relies on
// typeof storage[x] === "function" to determine whether the key x is set.
(sp) => {
  sp.storage = new Proxy({}, {
    get(target, prop) {
      if (prop in target) {
        return target[prop];
      }

      // Safety: Return undefined for specific symbols or 'then' to prevent
      // the Proxy from breaking async flows or inspection tools.
      // This is a breaking change from pre-SP3 behavior but improves safety.
      if (prop === 'then' || prop === 'toJSON' || typeof prop === 'symbol') {
        return undefined;
      }

      return () => { throw new Error(`Property "${prop}" is not defined in storage`); };
    },

    set(target, prop, value) {
      target[prop] = value;
      return true;
    }
  });
};
