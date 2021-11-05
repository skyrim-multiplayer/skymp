export class RespawnNeededError extends Error {
  constructor(message?: string) {
    super(message);
    Object.setPrototypeOf(this, RespawnNeededError.prototype);
  }
}
