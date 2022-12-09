import { Collection } from "./collection";

export interface IQueue<T> {
  enqueue(item: T): void;
  dequeue(): T | undefined;
  size(): number;
}

export class QueueCollection<T> extends Collection<T> implements IQueue<T> {
  constructor(private capacity: number = Infinity, initSize?: number) {
    super(initSize);
  }

  enqueue(item: T): void {
    if (this.isFull()) {
      throw Error("Queue has reached max capacity, you cannot add more items");
    }
    this.storage.push(item);
  }

  dequeue(): T | undefined {
    return this.storage.shift();
  }

  isFull(): boolean {
    return this.capacity === this.size();
  }
}
