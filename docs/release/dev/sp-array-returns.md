## Add support for arrays return in TS function of papyrus methods

Now we can get actual arrays return istead of Exception, when when using TS function for papyrus methods.

```typescript
Utility.createStringArray(3, "teststring");
// ["teststring", "teststring", "teststring"]
Utility.createBoolArray(3, true);
// [true, true, true]

const spell = player.getNthSpell(0);
browser.executeJavascript(`console.log(${JSON.stringify(s?.getEffectDurations())})`);
// prints numbers array to browser console [5, 5, 5];
const array = spell?.getMagicEffects();
// return array of PapyrusObjects
array?.forEach(element => {
    printConsole(MagicEffect.from(element)?.getFormID());
    // create MagicEffect from PapyrusObject with static .from
    // print FormID for all MagicEffects from 1st spell
})
```

Note, that we are alaways recieve PapyrusObject[] in case of object array return.
So we need to use static method `from` for elements to use class methods.
