import { GameModeController, SweetPie1000 } from "./_concept_deathMatch";

const makeControllerMock = (): GameModeController => {
  return { showMessageBox: jest.fn() };
}

describe("SweetPie", () => {
  test("Doesn't block activation of regular objects", () => {
    const sweetPie = new SweetPie1000(makeControllerMock());
    expect(sweetPie.onPlayerActivateObject(0xff000000, "1")).toEqual('continue');
  });

  test("Is able to connect player to DeathMatch", () => {
    const sweetPie = new SweetPie1000(makeControllerMock());
    
    // Player activates portal with E
    const activateRes = sweetPie.onPlayerActivateObject(0xff000000, sweetPie.portalNeutral);
    expect(activateRes).toEqual('blockActivation');
    expect(sweetPie.controller.showMessageBox).toBeCalledWith(0xff000000, sweetPie.dialogJoinDeathMatch, "DeathMatch", "Join DeathMatch?", ["Yes", "No"]);

    // Player selects Yes button
    sweetPie.onPlayerDialogResponse(0xff000000, sweetPie.dialogJoinDeathMatch, 0);
  });
});