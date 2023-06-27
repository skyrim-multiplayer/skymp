import { JsonSerializable } from './mp';
import * as skyrimPlatform from 'skyrimPlatform';

export interface Ctx<S = Record<string, JsonSerializable>, V = JsonSerializable> {
  /**
   * Refers to Skyrim Platform API.
   */
  readonly sp: typeof skyrimPlatform;

  /**
   * In `makeProperty` is always `undefined`.
   * In `updateOwner` is similar to `ctx.sp.Game.getPlayer()`.
   * In `updateNeighbor` refers to neighbor synchronized `ObjectReference` or `Actor`.
   */
  readonly refr?: skyrimPlatform.ObjectReference | skyrimPlatform.Actor | null;

  /**
   * In `makeProperty` is always `undefined`.
   * In `updateOwner` / `updateNeighbor` is equal to the value of a property that is
   * processed currently or `undefined` if there is no value or it's not visible due to flags.
   */
  readonly value?: V;

  /**
   * A writable object that is used to store data between `updateOwner`/`updateNeighbor`
   * calls or `makeProperty` initializations. `state` is currently shared between properties.
   */
  readonly state: S;

  /**
   * Get the value of the specified property. Built-in properties are not supported properly,
   * so attempts getting them are leading to the undefined behavior.
   * @param propertyName Name of the property we are reading.
   */
  readonly get?: (propertyName: string) => JsonSerializable | undefined;

  /**
   * Gets serverside formId by clientside formId or `0` if not found.
   * @param clientsideFormId Form ID of Actor or ObjectReference on the user's machine
   */
  readonly getFormIdInServerFormat: (clientsideFormId: number) => number;

  /**
   * Opposite to `getFormIdInServerFormat`. Gets clientside formId by serverside formId or
   * 0 if not found.
   * @param serversideFormId Form ID of MpActor or MpObjectReference in the server
   */
  readonly getFormIdInClientFormat: (serversideFormId: number) => number;

  /**
   * Respawns `ctx.refr` immediately.
   */
  readonly respawn: () => void;

  /**
   * Available only in `makeProperty` context. Sends an event to the server.
   * @param args Any serializable values that would be passed to the server handlers
   */
  readonly sendEvent: (...args: JsonSerializable[]) => void;
}
