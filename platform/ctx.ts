import { Actor } from './skyrimPlatform';
import { SkyrimPlatform } from './skyrimPlatformInterface';

export interface CTX {
	/**
	 * Skyrim Platform
	 */
	sp: SkyrimPlatform;
	/**
	 * Current Actor
	 */
	refr: Actor;
	value: any;
	/**
	 * storage value
	 */
	state: { [key: string]: any };
	/**
	 * Get the value form property by name
	 */
	get: (propertyName: string) => any;
	/**
	 * Get then ID of game object in server format
	 */
	getFormIdInServerFormat: (formId: number) => number;
	/**
	 * Get then ID of game object in client format
	 */
	getFormIdInClientFormat: (formId: number) => number;
	/**
	 * Respawn current Actor
	 */
	respawn: () => void;
	/**
	 * Send event to server side
	 */
	sendEvent: (...args: any) => void;
}
