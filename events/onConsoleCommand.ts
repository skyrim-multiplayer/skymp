import { MP } from '../types';

declare var mp: MP;

export const initConsoleCommandEvent = () => {
	mp.makeEventSource(
		'_onConsoleCommand',
		`ctx.sp.storage._api_onConsoleCommand = {
		    callback(...args) {
		      ctx.sendEvent(...args);
		    }
		  };
		`
	);
};
