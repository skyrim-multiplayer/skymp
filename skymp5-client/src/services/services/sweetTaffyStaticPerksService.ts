import { logTrace } from '../../logging';
import { ClientListener, CombinedController, Sp } from './clientListener';

export class SweetTaffyStaticPerksService extends ClientListener {
  constructor(private sp: Sp, private controller: CombinedController) {
    super();

    if (!this.hasSweetPie()) {
      logTrace(this, 'SweetTaffy features disabled');
    } else {
      logTrace(this, 'SweetTaffy features enabled');
    }

    this.controller.once('update', () => this.onceUpdate());
  }

  private onceUpdate() {
    if (!this.hasSweetPie()) {
      return;
    }

    this.addStaticPerksToThePlayer();
  }

  private addStaticPerksToThePlayer() {
    const player = this.sp.Game.getPlayer()!;

    const allStaticPerkIds = this.stealth
      .concat(this.magic)
      .concat(this.warrior);

    allStaticPerkIds.forEach((perkId) => {
      player.addPerk(this.sp.Perk.from(this.sp.Game.getFormEx(perkId)));
    });
  }

  private hasSweetPie(): boolean {
    const modCount = this.sp.Game.getModCount();
    for (let i = 0; i < modCount; ++i) {
      if (this.sp.Game.getModName(i).toLowerCase().includes('sweetpie')) {
        return true;
      }
    }
    return false;
  }

  // TODO: move this to config
  private readonly stealth = [0xbe126, 0xc07c6, 0xc07c7, 0xc07c8, 0xc07c9];
  private readonly magic = [0x58213, 0x105f24, 0x58214];
  private readonly warrior = [
    0xcb40d, 0xcb40f, 0xcb414, 0xcb411, 0xcb412, 0xcb410, 0xcb40e,
  ];
}
