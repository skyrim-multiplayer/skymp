/**
 * Built-in Properties
 */
type SystemPropertyName =
	| 'pos'
	| 'angle'
	| 'worldOrCellDesc'
	| 'inventory'
	| 'appearance'
	| 'isOpen'
	| 'isDisabled'
	| 'type'
	| 'baseDesc'
	| 'formDesc'
	| 'equipment'
	| 'isOnline';

/**
 * Custom Properties
 */
type CustomPropertyName =
	| 'isDead'
	| 'scale'
	| 'spawnPoint'
	| 'consoleOutput'
	| 'notification'
	| 'eval'
	| 'av_health'
	| 'av_magicka'
	| 'av_stamina'
	| 'av_healrate'
	| 'av_healratemult'
	| 'av_staminarate'
	| 'av_staminaratemult'
	| 'av_magickarate'
	| 'av_magickaratemult'
	| 'av_mp_healthdrain'
	| 'av_mp_magickadrain'
	| 'av_mp_staminadrain'
	| 'healthNumChanges'
	| 'magickaNumChanges'
	| 'staminaNumChanges';

export type PropertyName = SystemPropertyName | CustomPropertyName;
