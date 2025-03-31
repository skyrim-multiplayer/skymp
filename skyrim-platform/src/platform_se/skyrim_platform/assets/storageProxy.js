// Compat with pre-SP3 where storage is a Proxy like Actor/ObjectReference/etc are
// The user code incuding skymp5-client and skymp5-gamemode relies on typeof storage[x] === "function" to determine if x key is set
(sp) => {
  sp.storage = new Proxy({}, {
    get(target, prop) {
      if (prop in target) {
        return target[prop];
      }

      return () => { throw new Error(`Property "${prop}" is not defined in storage`); };
    },
    
    set(target, prop, value) {
      target[prop] = value;
      return true;
    }
  });
};
