import * as skyrimPlatform from './skyrimPlatform';

export interface Input {
	from: (form: skyrimPlatform.Form) => Input;
	getMappedControl: (keycode: number) => string;
	getMappedKey: (control: string, deviceType: number) => number;
	getNthKeyPressed: (n: number) => number;
	getNumKeysPressed: () => number;
	holdKey: (dxKeycode: number) => void;
	isKeyPressed: (dxKeycode: number) => boolean;
	releaseKey: (dxKeycode: number) => void;
	tapKey: (dxKeycode: number) => void;
}
