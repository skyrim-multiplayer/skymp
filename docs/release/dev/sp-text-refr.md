## Text Reference Attachment API

Texts created with `createText` can now be attached to in-world references (objects, NPCs, the player) and will automatically follow them on screen each frame.

### New functions

```typescript
// Attach text to a reference. Pass 0 to detach and restore the original screen position.
setTextRefr(textId: number, refrFormId: number): void;

// Set the skeleton node to track (e.g. "NPC Head [Head]"). When empty the
// object's base position (GetPosition) is used instead of the NiNode transform.
setTextRefrNode(textId: number, nodeName: string): void;

// Apply a world-space offset (x, y, z) on top of the tracked position.
setTextRefrOffset(textId: number, offset: number[]): void;

// Read back the currently set values.
getTextRefr(textId: number): number;
getTextRefrNode(textId: number): string;
getTextRefrOffset(textId: number): number[];
```

### Behaviour

- When `refrFormId` is non-zero the text's screen position is recalculated every update tick by projecting the world position through the player camera.
- If `refrNodeName` is empty the object's own position (`GetPosition()`) is used, so the text works even when the reference has no loaded 3D.
- If a named node is specified but cannot be found in the current frame the text position is not updated that frame.
- Texts behind the camera are not moved.
- Detaching (`setTextRefr(id, 0)`) restores the last screen position set via `setTextPos`.
