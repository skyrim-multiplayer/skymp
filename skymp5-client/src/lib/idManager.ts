export class IdManager {
  allocateIdFor(value: number): number {
    if (this.idByValue.length <= value) {
      this.idByValue.length = value + 1;
    }
    this.idByValue[value] = this.minimumUnusedId;

    if (this.valueById.length <= this.minimumUnusedId) {
      this.valueById.length = this.minimumUnusedId + 1;
    }
    this.valueById[this.minimumUnusedId] = value;

    const res = this.minimumUnusedId;
    this.minimumUnusedId++;
    while (
      this.valueById.length > this.minimumUnusedId &&
      typeof this.valueById[this.minimumUnusedId] === "number"
    ) {
      this.minimumUnusedId++;
    }
    return res;
  }

  freeIdFor(value: number): void {
    const id = this.idByValue[value] as number;
    if (id < this.minimumUnusedId) {
      this.minimumUnusedId = id;
    }
    this.idByValue[value] = undefined;
    this.valueById[id] = undefined;
    return;
  }

  getId(value: number): number {
    const r = this.idByValue[value];
    return typeof r === "number" ? r : -1;
  }

  getValueById(id: number): number | undefined {
    return this.valueById[id];
  }

  private idByValue = new Array<number | undefined>();
  private valueById = new Array<number | undefined>();
  private minimumUnusedId = 0;
}
