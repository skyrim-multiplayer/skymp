import { PersistentStorage } from './persistentStorage';

// This hacky utility is required since the server doesn't clear setTimeout during hot-reload
export class Timer {
  static init(): void {
    PersistentStorage.getSingleton().reloads++;
    const reloadsSnapshot = PersistentStorage.getSingleton().reloads;

    function timerCallback() {
      if (reloadsSnapshot !== PersistentStorage.getSingleton().reloads) {
        return;
      }
      try {
        Timer.everySecond();
      } catch (e) {
        throw e;
      } finally {
        setTimeout(timerCallback, 1000);
      }
    }

    setTimeout(timerCallback, 1000);
  }

  static everySecond: () => void = () => {};
}
