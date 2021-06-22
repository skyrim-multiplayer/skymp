const helloWorld = () => "hello world!";

describe("helloWorld", () => {
  it("should return 'hello world!'", () => {
    expect(helloWorld()).toEqual("hello world!");
  });
});
