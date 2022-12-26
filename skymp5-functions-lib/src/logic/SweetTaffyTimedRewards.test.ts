import { Counter } from "./PlayerController";
import { dayStart, SweetTaffyTimedRewards, TimedRewardController } from "./SweetTaffyTimedRewards";

export const mockController = () => {
  const counters = new Map<string, number>();
  return {
    addItem: jest.fn(),
    getOnlinePlayers: jest.fn(),
    getCurrentTime: jest.fn(),
    getCounter(actorId: number, counter: Counter) {
      return counters.get(actorId + ':' + counter) ?? 0;
    },
    setCounter(actorId: number, counter: Counter, to: number) {
      counters.set(actorId + ':' + counter, to);
    },
  };
};

describe("SweetTaffyTimedRewards", () => {
  test("dayStart works correctly", () => {
    expect(dayStart(new Date('2022-12-26T20:12:34.567+0300')).getTime())
      .toEqual(new Date('2022-12-26T00:00:00.000+0300').getTime());
    expect(dayStart(new Date('2022-12-26T00:00:00.000+0300')).getTime())
      .toEqual(new Date('2022-12-26T00:00:00.000+0300').getTime());
  });

  test("daily rewards", () => {
    const controller = mockController();
    const listener = new SweetTaffyTimedRewards(controller, /*enableDaily*/true, /*enableHourly*/false);

    controller.getCurrentTime.mockReturnValueOnce(new Date('2022-12-26T23:00:00.000+0300'));
    controller.getOnlinePlayers.mockReturnValue([1]);

    listener.everySecond();
    expect(controller.getCounter(1, 'everydayStart')).toEqual(new Date('2022-12-26T00:00:00.000+0300').getTime());
    expect(controller.getCounter(2, 'everydayStart')).toEqual(0);
    expect(controller.addItem).toBeCalledTimes(1);
    expect(controller.addItem).toBeCalledWith(1, SweetTaffyTimedRewards.rewardItemFormId, 150);

    controller.getCurrentTime.mockReturnValueOnce(new Date('2022-12-27T00:00:00.000+0300'));
    controller.getOnlinePlayers.mockReturnValue([1, 2]);
    controller.addItem.mockReset();
  
    listener.everySecond();
    expect(controller.getCounter(1, 'everydayStart')).toEqual(new Date('2022-12-27T00:00:00.000+0300').getTime());
    expect(controller.getCounter(2, 'everydayStart')).toEqual(new Date('2022-12-27T00:00:00.000+0300').getTime());
    expect(controller.addItem).toBeCalledTimes(2);
    expect(controller.addItem).toBeCalledWith(1, SweetTaffyTimedRewards.rewardItemFormId, 1);
    expect(controller.addItem).toBeCalledWith(2, SweetTaffyTimedRewards.rewardItemFormId, 150);

    controller.getCurrentTime.mockReturnValueOnce(new Date('2022-12-27T23:59:59.999+0300'));
    controller.addItem.mockReset();
  
    listener.everySecond();
    expect(controller.getCounter(1, 'everydayStart')).toEqual(new Date('2022-12-27T00:00:00.000+0300').getTime());
    expect(controller.getCounter(2, 'everydayStart')).toEqual(new Date('2022-12-27T00:00:00.000+0300').getTime());
    expect(controller.addItem).toBeCalledTimes(0);
  });

  test("hourly rewards", () => {
    const controller = mockController();
    const listener = new SweetTaffyTimedRewards(controller, /*enableDaily*/false, /*enableHourly*/true);

    controller.getCurrentTime.mockReturnValueOnce(new Date('2022-12-26T23:00:00.000+0300'));
    controller.getOnlinePlayers.mockReturnValue([1]);

    for (let i = 0; i < 60 * 60; ++i) {
      expect(controller.getCounter(1, 'secondsAccumulatedUntilHour')).toEqual(i);
      expect(controller.addItem).toBeCalledTimes(0);
      listener.everySecond();
    }
    expect(controller.getCounter(1, 'secondsAccumulatedUntilHour')).toEqual(0);
    expect(controller.addItem).toBeCalledWith(1, SweetTaffyTimedRewards.rewardItemFormId, 9);
  });
});
