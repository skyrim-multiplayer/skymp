/* eslint-disable @typescript-eslint/no-unused-vars */
import { register, localizationDefault } from './multiplayer';
import { Mp, PapyrusGlobalFunction } from '../types/mp';

const getRegisteredFunction = (
  callType: 'method' | 'global',
  fullName: string,
  registrations: ['method' | 'global', string, string, unknown][]
): PapyrusGlobalFunction => {
  const [className, funcName] = fullName.split('.');
  const matchingRegistrations = registrations
    .filter((x) => x[1] === className && x[2] === funcName)
    .filter((x) => x[0] == callType);
  if (matchingRegistrations.length !== 1) {
    throw new Error(`${fullName} has been defined ${matchingRegistrations.length} times while 1 required`);
  }
  const registration = matchingRegistrations[0];
  return registration[3] as PapyrusGlobalFunction;
};

const createMpMock = () => {
  const registerPapyrusFunction = jest.fn(
    (callType: 'method' | 'global', className: string, functionName: string, f: unknown): void => {
      return;
    }
  );
  const mp: Partial<Mp> = {
    registerPapyrusFunction,
  };

  const globals: Record<string, PapyrusGlobalFunction> = new Proxy(
    {},
    {
      get(target, key, receiver) {
        if (typeof key === 'string') {
          return getRegisteredFunction('global', key, registerPapyrusFunction.mock.calls);
        }
      },
    }
  );
  const methods: Record<string, PapyrusGlobalFunction> = new Proxy(
    {},
    {
      get(target, key, receiver) {
        if (typeof key === 'string') {
          return getRegisteredFunction('method', key, registerPapyrusFunction.mock.calls);
        }
      },
    }
  );

  return { mp, globals, methods };
};

describe('localizationDefault', () => {
  it('should return passed msgid itself', () => {
    expect(localizationDefault.getText('dkdkd')).toEqual('dkdkd');
  });
});

describe('ExecuteUiCommand', () => {
  it('sends a command to the UI', () => {
    // Setup test
    const { mp, globals } = createMpMock();
    const sendUiMessage = jest.fn((formId: number, msg: Record<string, unknown>): void => {
      return;
    });
    mp.sendUiMessage = sendUiMessage;
    mp.getIdFromDesc = () => 0xff000000;
    register(mp as Mp);

    // Run function
    globals['Multiplayer.ExecuteUiCommand'](null, [
      { desc: '0', type: 'form' },
      'SHOW_FACTION_TABLE',
      ['MEMBERS', 'RANK'],
      ['MEMBERS', 'q1000treadz', 'Sobaka', 'RANK', '10', '1'],
      'Faction members:\nq1000treadz(10)\nSobaka(1)',
    ]);

    // Verify results
    expect(sendUiMessage.mock.calls.length).toEqual(1);
    const [formId, msg] = sendUiMessage.mock.calls[0];
    expect(formId).toEqual(0xff000000);
    expect(msg).toEqual({
      type: 'COMMAND',
      data: {
        commandType: 'SHOW_FACTION_TABLE',
        commandArgs: {
          argumentNames: ['MEMBERS', 'RANK'],
          tokens: ['MEMBERS', 'q1000treadz', 'Sobaka', 'RANK', '10', '1'],
        },
        alter: ['Faction members:', 'q1000treadz(10)', 'Sobaka(1)'],
      },
    });
  });
});

describe('Log', () => {
  it('writes to console', () => {
    // Setup test
    const { mp, globals } = createMpMock();
    register(mp as Mp);
    console.log = jest.fn();

    // Run function
    globals['Multiplayer.Log'](null, ['Foo']);

    // Verify results
    expect(console.log).toHaveBeenCalledWith('[GM]', 'Foo');
  });
});

describe('Format', () => {
  it('inserts a string array to appropriate positions in base string', () => {
    // Setup test
    const { mp, globals } = createMpMock();
    register(mp as Mp);

    // Run function
    const res = globals['Multiplayer.Format'](null, ['%s   %s', ['Hello', 'Tamriel']]);

    // Verify results
    expect(res).toEqual('Hello   Tamriel');
  });
});

describe('GetText', () => {
  it('gets a localization string', () => {
    // Setup test
    const { mp, globals } = createMpMock();
    const getText = jest.fn((x) => 'wow');
    const localization = { getText };
    register(mp as Mp, localization);

    // Run function
    const res = globals['Multiplayer.GetText'](null, ['ddjiddjd']);

    // Verify results
    expect(res).toEqual('wow');
  });
});
