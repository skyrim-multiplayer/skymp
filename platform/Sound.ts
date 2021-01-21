import * as skyrimPlatform from './skyrimPlatform';

export interface Sound {
	from: (form: skyrimPlatform.Form) => Sound;
	getDescriptor: () => skyrimPlatform.SoundDescriptor;
	play: (akSource: skyrimPlatform.ObjectReference) => number;
	playAndWait: (akSource: skyrimPlatform.ObjectReference) => Promise<boolean>;
	setInstanceVolume: (aiPlaybackInstance: number, afVolume: number) => void;
	stopInstance: (aiPlaybackInstance: number) => void;
}
