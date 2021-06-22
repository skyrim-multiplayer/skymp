import { IdManager } from "../src/lib/idManager";

describe("IdManager", () => {
  it("should create ordered ids for values", () => {
    const m = new IdManager();
    expect(m.allocateIdFor(121)).toEqual(0);
    expect(m.allocateIdFor(213)).toEqual(1);
    expect(m.allocateIdFor(999)).toEqual(2);

    m.freeIdFor(213);
    expect(m.allocateIdFor(11111)).toEqual(1);
    expect(m.allocateIdFor(11112)).toEqual(3);

    expect(m.getId(121)).toEqual(0);
    expect(m.getId(213)).toEqual(-1);
    expect(m.getId(999)).toEqual(2);
    expect(m.getId(11111)).toEqual(1);
    expect(m.getId(11112)).toEqual(3);

    expect(m.getValueById(0)).toEqual(121);
  });
});
