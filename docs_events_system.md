# Events System

Events system allows you to handle events existing in the multiplayer and add your own custom events.

```typescript
// 1) Making an event
// Custom event names have to start with underscore
mp.makeEventSource("_onSomeEvent", `
    ctx.sp.once("update", () => {
      ctx.sendEvent();
    });
  `);
);

// 2) Adding a handler for the event
mp._onSomeEvent = (pcFormId) => console.log("handled for", pcFormId); ;
```

## Built-in events

- `onInit(formId)` - an event fired each time `MpObjectReference` / `MpActor` is initializing from scratch or from a database record. May be called multiple times for the same `formId` during server run.

- `onUiEvent(formId, data)` - an event fired each time a UI event is being sent in the browser context of the client.
