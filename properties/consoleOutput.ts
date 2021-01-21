import { MP, PropertyName } from '../types';

declare const mp: MP;

const genericPrint = (propName: PropertyName, formId: number, ...printConsoleArgs: any[]) => {
	const prev = mp.get(formId, propName);
	const n = prev ? prev.n : 0;
	mp.set(formId, propName, {
		n: n + 1,
		args: printConsoleArgs,
		date: +Date.now(),
	});
};

export type printTargetsPropName = 'consoleOutput' | 'notification' | 'eval';

export const consoleOutput = {
	print: (formId: number, ...args: any[]) => genericPrint('consoleOutput', formId, ...args),
	printNote: (formId: number, ...args: any[]) => genericPrint('notification', formId, ...args),
	evalClient: (formId: number, ...args: any[]) => genericPrint('eval', formId, ...args),
};

const printTargets: { [key: string]: string } = {
	consoleOutput: 'ctx.sp.printConsole(...ctx.value.args);',
	notification: 'ctx.sp.Debug.notification(...ctx.value.args);',
	eval: 'eval(...ctx.value.args)',
};
const props: PropertyName[] = ['consoleOutput', 'notification', 'eval'];

export const initConsoleOutput = () => {
	for (const propName of props) {
		const updateOwner = () => `
      if (ctx.state.n${propName} === ctx.value.n) return;
      ctx.state.n${propName} = ctx.value.n;
      if (ctx.value.date) {
        if (Math.abs(ctx.value.date - +Date.now()) > 2000) return;
      }
      ${printTargets[propName]}
    `;
		mp.makeProperty(propName, {
			isVisibleByOwner: true,
			isVisibleByNeighbors: false,
			updateNeighbor: '',
			updateOwner: updateOwner(),
		});
	}
};
