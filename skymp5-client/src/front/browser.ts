import {
  browser,
  on,
  once,
  Input,
  printConsole,
  settings,
  Ui,
  UiMenu,
  DxScanCodes
} from "skyrimPlatform";
import { DXScanCodes } from '../lib/dx-scan-codes';
import { EventEmitter } from '../lib/event-emitter';
import { badMenus } from '../lib/ui-menu';

export const main = (): void => {
  const emitter = new EventEmitter();

	const badMenus: UiMenu[] = [
		UiMenu.Barter,
		UiMenu.Book,
		UiMenu.Container,
		UiMenu.Crafting,
		UiMenu.Gift,
		UiMenu.Inventory,
		UiMenu.Journal,
		UiMenu.Lockpicking,
		UiMenu.Loading,
		UiMenu.Map,
		UiMenu.RaceSex,
		UiMenu.Stats,
		UiMenu.Tween,
		UiMenu.Console,
		UiMenu.Main,
	];

	let browserVisibleState = false;
	browser.setVisible(false);
	const setBrowserVisible = (state: boolean) => {
		browserVisibleState = state;
		browser.setVisible(state);
	};
	once('update', () => {
		browserVisibleState = true;
		browser.setVisible(true);
	});

	let browserFocusedState = false;
	const setBrowserFocused = (state: boolean) => {
		browserFocusedState = state;
		browser.setFocused(state);
	};

	let badMenuOpen = true;

	let lastBadMenuCheck = 0;
	on('update', () => {
		if (Date.now() - lastBadMenuCheck > 200) {
			lastBadMenuCheck = Date.now();
			badMenuOpen = badMenus.findIndex((menu) => Ui.isMenuOpen(menu)) !== -1;
			browser.setVisible(browserVisibleState && !badMenuOpen);
		}
	});

	const onInputChangeEventName = 'event:input-change';

	const singleBindings: Record<number, () => void> = {
		[DxScanCodes.F2]: () => setBrowserVisible(!browserVisibleState),
		[DxScanCodes.F6]: () => setBrowserFocused(!browserFocusedState),
		[DxScanCodes.Escape]: () => (browserFocusedState ? setBrowserFocused(false) : undefined)
	};

	const getMultiBindigString = (...codes: DxScanCodes[]) => codes.sort((a, b) => a - b).join('-');
	const multiBindings: Record<string, () => void> = {
		[getMultiBindigString(DxScanCodes.LeftShift, DxScanCodes.Tab)]: () => {
		  // example
		},
	};

	let lastNumKeys = 0;
	on('update', () => {
		const numKeys = Input.getNumKeysPressed();

		if (lastNumKeys !== numKeys) {
			lastNumKeys = numKeys;

			const keyCodes = Array(numKeys)
				.fill(null)
				.map((_, i) => Input.getNthKeyPressed(i));

			emitter.emit(onInputChangeEventName, keyCodes);
		}
	});

	emitter.subscribe(onInputChangeEventName, (data) => {
		if (!Array.isArray(data)) return;

		const keycodes: number[] = data;
		if (keycodes.length === 0) return;

		const multi: string = keycodes.join('-');

		if (multiBindings[multi]) {
			multiBindings[multi]();
			return;
		}

		const single: number = keycodes[0];
		if (singleBindings[single]) singleBindings[single]();
	});

  const cfg = {
    ip: settings["skymp5-client"]["server-ip"],
    port: settings["skymp5-client"]["server-port"],
  };

  printConsole({ cfg });

  const uiPort = cfg.port === 7777 ? 3000 : cfg.port as number + 1;

  const url = `http://${cfg.ip}:${uiPort}/ui/index.html`;
  printConsole(`loading url ${url}`);
  browser.loadUrl(url);
};
