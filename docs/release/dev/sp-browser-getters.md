## Add `browser.isVisible`/`browser.isFocused` methods

Now we can get an actual state of visible/focused flags instead of remembering it each time we do `setVisible`/`setFocused`.

```typescript
printConsole("isVisible:", browser.isVisible());
printConsole("isFocused:", browser.isFocused());
```
