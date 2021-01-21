import * as skyrimPlatform from './skyrimPlatform';

export interface Debug {
	from: (form: skyrimPlatform.Form) => Debug;
	centerOnCell: (param1: string) => void;
	centerOnCellAndWait: (param1: string) => Promise<number>;
	closeUserLog: (param1: string) => void;
	dBSendPlayerPosition: () => void;
	debugChannelNotify: (param1: string, param2: string) => void;
	dumpAliasData: (param1: skyrimPlatform.Quest) => void;
	getConfigName: () => Promise<string>;
	getPlatformName: () => Promise<string>;
	getVersionNumber: () => Promise<string>;
	messageBox: (param1: string) => void;
	notification: (param1: string) => void;
	openUserLog: (param1: string) => boolean;
	playerMoveToAndWait: (param1: string) => Promise<number>;
	quitGame: () => void;
	sendAnimationEvent: (
		param1: skyrimPlatform.ObjectReference,
		param2: string
	) => void;
	setFootIK: (param1: boolean) => void;
	setGodMode: (param1: boolean) => void;
	showRefPosition: (arRef: skyrimPlatform.ObjectReference) => void;
	startScriptProfiling: (param1: string) => void;
	startStackProfiling: () => void;
	stopScriptProfiling: (param1: string) => void;
	stopStackProfiling: () => void;
	takeScreenshot: (param1: string) => void;
	toggleAI: () => void;
	toggleCollisions: () => void;
	toggleMenus: () => void;
	trace: (param1: string, param2: number) => void;
	traceStack: (param1: string, param2: number) => void;
	traceUser: (param1: string, param2: string, param3: number) => boolean;
}
