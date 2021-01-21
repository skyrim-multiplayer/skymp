import { defaultSpawnPoint } from '../constants';
import { spawnSystem } from '../systems';
import { MP } from '../types';
import { utils } from '../utility';

declare const mp: MP;

export const initSpawnPoint = () => {
	mp.makeProperty('spawnPoint', {
		isVisibleByOwner: false,
		isVisibleByNeighbors: false,
		updateNeighbor: '',
		updateOwner: '',
	});

	utils.hook('onDeath', (pcFormId: number) => {
		setTimeout(() => {
			spawnSystem.spawn(pcFormId);
		}, spawnSystem.timeToRespawn);
	});

	utils.hook('onReinit', (pcFormId: number, options: any) => {
		/** set respawn point */
		if (!mp.get(pcFormId, 'spawnPoint') || (options && options.force)) {
			mp.set(pcFormId, 'spawnPoint', defaultSpawnPoint);
		}
	});
};
