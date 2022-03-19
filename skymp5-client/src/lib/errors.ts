export class RespawnNeededError extends Error {
  constructor(message?: string) {
    super(message);
    Object.setPrototypeOf(this, RespawnNeededError.prototype);
  }
}

export class NeverError extends Error {
  constructor(message: never) {
    super(`Unreachable statement: ${message}`);
  }
}
