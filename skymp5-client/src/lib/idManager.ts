import { printConsole } from 'skyrimPlatform';

import { QueueCollection } from '../collections/queue';
import { nameof } from './nameof';

export class IdManager {
  public static readonly CLASS_NAME = 'IdManager';
  public static readonly FREE_IDS_CAPACITY = 2000;

  allocateIdFor(value: number): number {
    const id = this.idByValue.get(value);
    if (id) {
      printConsole(
        `WARNING: ${IdManager.CLASS_NAME}.${nameof<IdManager>(
          'allocateIdFor',
        )} found duplicate value = "${value}"`,
      );
      return id;
    }

    let newId = this.freeIds.dequeue() ?? ++this.lastMaxId;
    this.idByValue.set(value, newId);
    this.valueById.set(newId, value);
    return newId;
  }

  freeIdFor(value: number): void {
    const id = this.idByValue.get(value);
    if (!id) {
      printConsole(
        `WARNING: ${IdManager.CLASS_NAME}.${nameof<IdManager>(
          'freeIdFor',
        )} not found value = "${value}"`,
      );
      return;
    }

    this.idByValue.delete(value);
    this.valueById.delete(id);

    if (!this.freeIds.isFull()) {
      this.freeIds.enqueue(id);
    }
  }

  getId(value: number): number {
    return this.idByValue.get(value) ?? -1;
  }

  getValueById(id: number): number | undefined {
    return this.valueById.get(id);
  }

  private idByValue = new Map<number, number>();
  private valueById = new Map<number, number>();
  private lastMaxId = 0;
  private freeIds = new QueueCollection<number>(IdManager.FREE_IDS_CAPACITY);
}
