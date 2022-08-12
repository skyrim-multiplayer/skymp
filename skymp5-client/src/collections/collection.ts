export abstract class Collection<T> implements Iterable<T> {
  constructor(initSize?: number) {
    this.storage = new Array<T>(initSize ?? 0);
  }

  protected storage: T[];

  [Symbol.iterator](): Iterator<T, any, undefined> {
    let counter = 0;
    return {
      next: () => {
        return {
          done: counter >= this.storage.length,
          value: this.storage[counter++]
        }
      }
    }
  }

  size(): number {
    return this.storage.length;
  }

  abstract isFull(): boolean;
}
