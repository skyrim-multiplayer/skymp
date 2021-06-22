import { LocalizationProvider } from './localizationProvider';
import * as fs from 'fs';
import * as path from 'path';
import * as os from 'os';

const prepareFile = () => {
  const p = path.join(os.tmpdir(), 'tmp_skymp_locale.po');
  const localization = {
    'Unknown command %s': 'Неизвестная команда %s',
  };
  fs.writeFileSync(p, JSON.stringify(localization));
  return p;
};

describe('LocalizationProvider', () => {
  it('should be able to get localization strings from a valid file', () => {
    const p = prepareFile();
    const localizationProvider = new LocalizationProvider(p, 'once');

    expect(localizationProvider.getText('Unknown command %s')).toEqual('Неизвестная команда %s');

    // Damage file and verify that localization has been cached
    fs.writeFileSync(p, 'foo');
    expect(localizationProvider.getText('Unknown command %s')).toEqual('Неизвестная команда %s');
  });

  it('should be able to get localization strings from a valid file', () => {
    const p = prepareFile();
    const localizationProvider = new LocalizationProvider(p, 'hotreload');

    expect(localizationProvider.getText('Unknown command %s')).toEqual('Неизвестная команда %s');

    // Damage file and verify that the default localization would be used
    fs.writeFileSync(p, 'foo');
    expect(localizationProvider.getText('Unknown command %s')).toEqual('Unknown command %s');

    // Verify that hotreload works
    prepareFile();
    expect(localizationProvider.getText('Unknown command %s')).toEqual('Неизвестная команда %s');
  });

  it('should return msgid itself when the file does not exist', () => {
    const localizationProvider = new LocalizationProvider('INVALID PATH', 'once');

    expect(localizationProvider.getText('Unknown command %s')).toEqual('Unknown command %s');
  });
});
