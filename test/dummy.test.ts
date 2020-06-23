import { helloWorld } from "../src/lib/helloWorld";

describe("helloWorld", () => {
  it("should return 'hello world!'", () => {
    expect(helloWorld()).toEqual("hello world!");
  });
});
