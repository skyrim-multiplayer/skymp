import { EventName } from '../types/EventName';
import { PropertyName } from '../types/PropertyName';

/* Definition */
interface MakePropertyOptions {
	// If set to false, `updateOwner` would never be invoked
	// Player's client wouldn't see it's own value of this property
	// Reasonable for passwords and other secret values
	isVisibleByOwner: boolean;

	// If set to false, `updateNeighbor` would never be invoked
	// Player's client wouldn't see values of neighbor Actors/ObjectReferences
	isVisibleByNeighbors: boolean;

	// Body of functions that would be invoked on client every update.
	updateOwner: string; // For the PlayerCharacter
	updateNeighbor: string; // For each synchronized Actor/ObjectReference
}

export interface MP {
	/**
	 * Create property on server
	 * @param propertyName property name
	 * @param options options for property
	 */
	makeProperty(propertyName: PropertyName, options: MakePropertyOptions): void;
	/**
	 * Create event on server
	 * @param eventName event name
	 * @param functionBody function that trigger when event call
	 */
	makeEventSource(eventName: EventName, functionBody: string): void;
	/**
	 * Get the value from property by name
	 * @param formId unique identifier
	 * @param propertyName property name
	 */
	get(formId: number, propertyName: PropertyName): any;
	/**
	 * Set the new value to the property
	 * @param formId unique identifier
	 * @param propertyName property name
	 * @param newValue new value for property
	 */
	set(formId: number, propertyName: PropertyName, newValue: any): void;
	/**
	 * clear
	 */
	clear(): void;

	/**
	 * Send a message to the running embedded browser
	 * @param formId unique identifier
	 * @param message serializable object representing a message
	 */
	sendUiMessage(formId: number, message: Record<string, unknown>): void;

	[key: string]: (...args: any[]) => void;
}
