/* eslint-disable @typescript-eslint/no-var-requires */
const assert = require('node:assert/strict');
const React = require('react');
const { renderToStaticMarkup } = require('react-dom/server');

process.env.TS_NODE_COMPILER_OPTIONS = JSON.stringify({
  module: 'commonjs',
  moduleResolution: 'node',
  jsx: 'react',
  esModuleInterop: true,
  allowSyntheticDefaultImports: true,
});

require('ts-node/register/transpile-only');

require.extensions['.scss'] = () => null;
require.extensions['.svg'] = (module, filename) => {
  module.exports = {
    __esModule: true,
    default: filename,
  };
};

require('../src/i18n.ts');
const i18n = require('../src/i18n.ts').default;

const {
  getVisibleServers,
  collectServerTags,
  pingClass,
  pingLabel,
  isValidPort,
  isValidHostOrIp,
} = require('../src/features/serverList/utils.ts');
const {
  getLauncherIgnoredUpdateVersion,
  setLauncherIgnoredUpdateVersion,
  clearLauncherIgnoredUpdateVersion,
} = require('../src/features/serverList/preferences.ts');
const {
  filterAdminPlayers,
  formatAdminPos,
  formatAdminUptime,
  formatAdminTime,
  MAX_MODERATION_REASON_LENGTH,
  isValidModerationReason,
  isValidDiscordSnowflake,
  isValidWhitelistIdentifier,
} = require('../src/features/adminDashboard/utils.ts');
const {
  detectLanguage,
  detectRuntimeLanguage,
} = require('../src/utils/i18nLanguage.ts');
const {
  getFrameButtonClassName,
  shouldHandleFrameButtonClick,
} = require('../src/components/FrameButton/utils.ts');
const {
  FrameButton,
} = require('../src/components/FrameButton/FrameButton.tsx');
const {
  ImageButton,
} = require('../src/components/ImageButton/ImageButton.tsx');
const { SkyrimHint } = require('../src/components/SkyrimHint/SkyrimHint.tsx');
const {
  SkyrimSlider,
} = require('../src/components/SkyrimSlider/SkyrimSlider.tsx');
const {
  SkyrimFrame,
} = require('../src/components/SkyrimFrame/SkyrimFrame.tsx');
const {
  SkyrimButton,
} = require('../src/components/SkyrimButton/SkyrimButton.tsx');
const {
  SkyrimInput,
} = require('../src/components/SkyrimInput/SkyrimInput.tsx');
const {
  AppErrorBoundary,
} = require('../src/components/AppErrorBoundary/AppErrorBoundary.tsx');
const AdminDashboard = require('../src/features/adminDashboard').default;

const run = (name, fn) => {
  try {
    fn();
    console.log(`[OK] ${name}`);
  } catch (error) {
    console.error(`[FAIL] ${name}`);
    throw error;
  }
};

const render = (component) => renderToStaticMarkup(component);

run('AdminDashboard initializes while hidden without crashing the app', () => {
  assert.equal(render(React.createElement(AdminDashboard)), '');
});

run('pingClass maps ping buckets', () => {
  assert.equal(pingClass(null), 'server-list__ping--unknown');
  assert.equal(pingClass(20), 'server-list__ping--good');
  assert.equal(pingClass(80), 'server-list__ping--good');
  assert.equal(pingClass(81), 'server-list__ping--ok');
  assert.equal(pingClass(150), 'server-list__ping--ok');
  assert.equal(pingClass(151), 'server-list__ping--bad');
});

run('pingLabel formats values', () => {
  assert.equal(pingLabel(null), '–');
  assert.equal(pingLabel(0), '0ms');
  assert.equal(pingLabel(95), '95ms');
});

run('isValidPort validates integer TCP port range', () => {
  assert.equal(isValidPort(1), true);
  assert.equal(isValidPort(65535), true);
  assert.equal(isValidPort(0), false);
  assert.equal(isValidPort(65536), false);
  assert.equal(isValidPort(7777.5), false);
});

run('isValidHostOrIp validates hostnames and IPv4 addresses', () => {
  assert.equal(isValidHostOrIp('127.0.0.1'), true);
  assert.equal(isValidHostOrIp('255.255.255.255'), true);
  assert.equal(isValidHostOrIp('999.10.1.5'), false);
  assert.equal(isValidHostOrIp('skymp.local'), true);
  assert.equal(isValidHostOrIp('invalid host name'), false);
});

run('collectServerTags returns unique sorted tags', () => {
  const servers = [
    {
      id: '1',
      name: 'A',
      ip: '1.1.1.1',
      players: 1,
      maxPlayers: 2,
      ping: 10,
      tags: ['pve', 'eu'],
    },
    {
      id: '2',
      name: 'B',
      ip: '1.1.1.2',
      players: 1,
      maxPlayers: 2,
      ping: 20,
      tags: ['pvp', 'eu', ''],
    },
    { id: '3', name: 'C', ip: '1.1.1.3', players: 1, maxPlayers: 2, ping: 30 },
  ];

  assert.deepEqual(collectServerTags(servers), ['eu', 'pve', 'pvp']);
});

run('getVisibleServers filters full servers and search query', () => {
  const servers = [
    { name: 'Alpha', ip: '10.0.0.1', players: 10, maxPlayers: 20, ping: 90 },
    { name: 'Beta', ip: '10.0.0.2', players: 20, maxPlayers: 20, ping: 50 },
    {
      name: 'Gamma',
      ip: '192.168.1.5',
      players: 4,
      maxPlayers: 10,
      ping: null,
    },
  ];

  const noFull = getVisibleServers(servers, '', 'players', false);
  assert.deepEqual(
    noFull.map((s) => s.name),
    ['Alpha', 'Gamma'],
  );

  const searchByName = getVisibleServers(servers, 'ga', 'players', true);
  assert.deepEqual(
    searchByName.map((s) => s.name),
    ['Gamma'],
  );

  const searchByIp = getVisibleServers(servers, '10.0.0.2', 'players', true);
  assert.deepEqual(
    searchByIp.map((s) => s.name),
    ['Beta'],
  );
});

run('getVisibleServers sorting respects players, ping and name', () => {
  const servers = [
    { name: 'Charlie', ip: '10.0.0.3', players: 3, maxPlayers: 20, ping: null },
    { name: 'Alpha', ip: '10.0.0.1', players: 8, maxPlayers: 20, ping: 70 },
    { name: 'Bravo', ip: '10.0.0.2', players: 5, maxPlayers: 20, ping: 40 },
  ];

  const byPlayers = getVisibleServers(servers, '', 'players', true);
  assert.deepEqual(
    byPlayers.map((s) => s.name),
    ['Alpha', 'Bravo', 'Charlie'],
  );

  const byPing = getVisibleServers(servers, '', 'ping', true);
  assert.deepEqual(
    byPing.map((s) => s.name),
    ['Bravo', 'Alpha', 'Charlie'],
  );

  const byName = getVisibleServers(servers, '', 'name', true);
  assert.deepEqual(
    byName.map((s) => s.name),
    ['Alpha', 'Bravo', 'Charlie'],
  );
});

run('getVisibleServers supports favorites and tag filters', () => {
  const servers = [
    {
      id: '1',
      name: 'Alpha',
      ip: '10.0.0.1',
      players: 2,
      maxPlayers: 20,
      ping: 90,
      tags: ['pve'],
    },
    {
      id: '2',
      name: 'Beta',
      ip: '10.0.0.2',
      players: 3,
      maxPlayers: 20,
      ping: 50,
      tags: ['pvp'],
    },
    {
      id: '3',
      name: 'Gamma',
      ip: '10.0.0.3',
      players: 4,
      maxPlayers: 20,
      ping: 70,
      tags: ['pve', 'hardcore'],
    },
  ];

  const favoriteIds = new Set(['2', '3']);
  const onlyFavorites = getVisibleServers(servers, '', 'name', true, {
    favoriteIds,
    onlyFavorites: true,
  });
  assert.deepEqual(
    onlyFavorites.map((s) => s.id),
    ['2', '3'],
  );

  const byTag = getVisibleServers(servers, '', 'players', true, {
    requiredTag: 'pve',
  });
  assert.deepEqual(
    byTag.map((s) => s.id),
    ['3', '1'],
  );
});

run('detectLanguage resolves supported and fallback locales', () => {
  assert.equal(detectLanguage('en-US'), 'en');
  assert.equal(detectLanguage('ru-RU'), 'ru');
  assert.equal(detectLanguage('de-DE'), 'de');
  assert.equal(detectLanguage('es-ES'), 'es');
  assert.equal(detectLanguage('fr-FR'), 'fr');
  assert.equal(detectLanguage('it-IT'), 'it');
  assert.equal(detectLanguage(undefined), 'en');
});

run(
  'detectRuntimeLanguage falls back to English when navigator is unavailable',
  () => {
    const originalNavigator = global.navigator;

    Object.defineProperty(global, 'navigator', {
      value: undefined,
      configurable: true,
      writable: true,
    });

    try {
      assert.equal(detectRuntimeLanguage(), 'en');
    } finally {
      Object.defineProperty(global, 'navigator', {
        value: originalNavigator,
        configurable: true,
        writable: true,
      });
    }
  },
);

run('FrameButton helpers map disabled and clickable state', () => {
  assert.equal(getFrameButtonClassName(true), 'skymp-button disabled');
  assert.equal(getFrameButtonClassName(false), 'skymp-button active');
  assert.equal(shouldHandleFrameButtonClick(true), false);
  assert.equal(shouldHandleFrameButtonClick(false), true);
});

run('launcher ignored update version stores and clears by channel', () => {
  const previousWindow = global.window;
  const storage = {};

  global.window = {
    localStorage: {
      getItem: (key) =>
        Object.prototype.hasOwnProperty.call(storage, key)
          ? storage[key]
          : null,
      setItem: (key, value) => {
        storage[key] = String(value);
      },
      removeItem: (key) => {
        delete storage[key];
      },
    },
  };

  try {
    assert.equal(getLauncherIgnoredUpdateVersion('stable'), null);
    setLauncherIgnoredUpdateVersion('stable', '1.2.0');
    setLauncherIgnoredUpdateVersion('beta', '1.3.0-beta.1');

    assert.equal(getLauncherIgnoredUpdateVersion('stable'), '1.2.0');
    assert.equal(getLauncherIgnoredUpdateVersion('beta'), '1.3.0-beta.1');

    clearLauncherIgnoredUpdateVersion('stable');
    assert.equal(getLauncherIgnoredUpdateVersion('stable'), null);
    assert.equal(getLauncherIgnoredUpdateVersion('beta'), '1.3.0-beta.1');
  } finally {
    if (previousWindow === undefined) {
      delete global.window;
    } else {
      global.window = previousWindow;
    }
  }
});

run('formatAdminUptime formats seconds, minutes and hours', () => {
  assert.equal(formatAdminUptime(42), '42s');
  assert.equal(formatAdminUptime(65), '1m 5s');
  assert.equal(formatAdminUptime(3665), '1h 1m');
  assert.equal(formatAdminUptime(0), '0s');
  assert.equal(formatAdminUptime(3600), '1h 0m');
});

run('formatAdminPos formats object/array/empty values', () => {
  assert.equal(formatAdminPos({ x: 10.4, y: 20.6, z: -3.1 }), '10, 21, -3');
  assert.equal(formatAdminPos([1.2, 2.8, 3.5]), '1, 3, 4');
  assert.equal(formatAdminPos(undefined), '-');
});

run('filterAdminPlayers matches by userId, actorName and ip', () => {
  const players = [
    { userId: 12, actorName: 'Dovahkiin', ip: '127.0.0.1' },
    { userId: 33, actorName: 'Vilkas', ip: '10.0.0.5' },
    { userId: 77, actorName: 'Serana', ip: '192.168.0.77' },
  ];

  assert.deepEqual(
    filterAdminPlayers(players, '12').map((p) => p.userId),
    [12],
  );
  assert.deepEqual(
    filterAdminPlayers(players, 'vilk').map((p) => p.userId),
    [33],
  );
  assert.deepEqual(
    filterAdminPlayers(players, '192.168').map((p) => p.userId),
    [77],
  );
  assert.deepEqual(
    filterAdminPlayers(players, '').map((p) => p.userId),
    [12, 33, 77],
  );
});

run('isValidDiscordSnowflake accepts digits and empty values only', () => {
  assert.equal(isValidDiscordSnowflake(''), true);
  assert.equal(isValidDiscordSnowflake('  '), true);
  assert.equal(isValidDiscordSnowflake('272600190639848628'), true);
  assert.equal(isValidDiscordSnowflake(' 272600190639848628 '), true);
  assert.equal(isValidDiscordSnowflake('discord:272600190639848628'), false);
  assert.equal(isValidDiscordSnowflake('abc123'), false);
});

run('isValidModerationReason keeps optional reason but caps max length', () => {
  assert.equal(isValidModerationReason(''), true);
  assert.equal(isValidModerationReason('   '), true);
  assert.equal(
    isValidModerationReason('x'.repeat(MAX_MODERATION_REASON_LENGTH)),
    true,
  );
  assert.equal(
    isValidModerationReason('x'.repeat(MAX_MODERATION_REASON_LENGTH + 1)),
    false,
  );
});

run('isValidWhitelistIdentifier mirrors the backend whitelist contract', () => {
  assert.equal(isValidWhitelistIdentifier('discord:272600190639848628'), true);
  assert.equal(isValidWhitelistIdentifier(' LICENSE : abc123 '), true);
  assert.equal(isValidWhitelistIdentifier('steam:110000104cbe442'), true);
  assert.equal(isValidWhitelistIdentifier(''), false);
  assert.equal(isValidWhitelistIdentifier('discord'), false);
  assert.equal(isValidWhitelistIdentifier('discord:'), false);
  assert.equal(isValidWhitelistIdentifier(':272600190639848628'), false);
  assert.equal(isValidWhitelistIdentifier('discordia:272600190639848628'), false);
  assert.equal(isValidWhitelistIdentifier('licenseea'), false);
});

run(
  'formatAdminTime returns a non-empty time string for a known timestamp',
  () => {
    // fixed UTC timestamp: 2024-01-15T12:34:56.000Z
    const ts = 1705319696000;
    const result = formatAdminTime(ts);
    assert.equal(typeof result, 'string');
    assert.equal(result.length > 0, true);
  },
);

run('AppErrorBoundary renders localized fallback UI after an error', () => {
  const boundary = new AppErrorBoundary({
    children: React.createElement('div', null, 'ok'),
  });

  boundary.state = AppErrorBoundary.getDerivedStateFromError(new Error('boom'));

  const html = render(boundary.render());
  assert.match(html, /data-testid="app-error-boundary"/);
  assert.equal(html.includes(i18n.t('appErrorBoundary.title')), true);
  assert.equal(html.includes(i18n.t('appErrorBoundary.reload')), true);
});

run('SkyrimButton renders text and disabled opacity', () => {
  const html = render(
    React.createElement(SkyrimButton, {
      name: 'loginSubmit',
      text: 'Login',
      width: 300,
      height: 50,
      disabled: true,
      onClick: () => {},
    }),
  );

  assert.equal(html.includes('skymp-input-button'), true);
  assert.equal(html.includes('Login'), true);
  assert.equal(html.includes('opacity:0.6'), true);
  assert.equal(html.includes('width:300px'), true);
  assert.equal(html.includes('height:50px'), true);
});

run('SkyrimInput renders label, placeholder and initial value', () => {
  const html = render(
    React.createElement(SkyrimInput, {
      name: 'email',
      type: 'email',
      labelText: 'E-Mail',
      placeholder: 'name@example.com',
      initialValue: 'hero@skymp.org',
      width: 280,
      height: 44,
      onInput: () => {},
    }),
  );

  assert.equal(html.includes('E-Mail'), true);
  assert.equal(html.includes('placeholder="name@example.com"'), true);
  assert.equal(html.includes('value="hero@skymp.org"'), true);
  assert.equal(html.includes('name="email"'), true);
  assert.equal(html.includes('width:280px'), true);
  assert.equal(html.includes('height:44px'), true);
});

run('FrameButton renders default variant with active state and text', () => {
  const html = render(
    React.createElement(FrameButton, {
      name: 'playBtn',
      text: 'Play',
      variant: 'DEFAULT',
      width: 384,
      height: 64,
      disabled: false,
      onClick: () => {},
    }),
  );

  assert.equal(html.includes('class="skymp-button active"'), true);
  assert.equal(html.includes('Play'), true);
  assert.equal(html.includes('width:384px'), true);
  assert.equal(html.includes('height:64px'), true);
  assert.equal(html.includes('skyrim-button-start'), true);
  assert.equal(html.includes('button-middle'), true);
  assert.equal(html.includes('skyrim-button-end'), true);
});

run('ImageButton renders image and disabled button style', () => {
  const html = render(
    React.createElement(ImageButton, {
      src: '/icons/discord.svg',
      width: 320,
      height: 48,
      disabled: true,
      onClick: () => {},
    }),
  );

  assert.equal(html.includes('login-form--content_social__link'), true);
  assert.equal(html.includes('src="/icons/discord.svg"'), true);
  assert.equal(html.includes('opacity:0.6'), true);
  assert.equal(html.includes('cursor:default'), true);
});

run('SkyrimHint renders open left-aligned active state', () => {
  const html = render(
    React.createElement(SkyrimHint, {
      isOpened: true,
      text: 'Connect to selected server',
      active: true,
      left: true,
    }),
  );

  assert.equal(html.includes('skymp-hint active left'), true);
  assert.equal(html.includes('Connect to selected server'), true);
  assert.equal(html.includes('display:flex'), true);
  assert.equal(html.includes('hint.svg'), true);
});

run('SkyrimSlider renders title and slider classes', () => {
  const html = render(
    React.createElement(SkyrimSlider, {
      text: 'Volume',
      sliderValue: 35,
      min: 0,
      max: 100,
      marks: false,
      setValue: () => {},
    }),
  );

  assert.equal(html.includes('skyrimSlider'), true);
  assert.equal(html.includes('skyrimSlider_text'), true);
  assert.equal(html.includes('Volume'), true);
  assert.equal(html.includes('skyrimSlider_slider'), true);
  assert.equal(html.includes('skyrimSlider_thumb'), true);
});

run('SkyrimFrame renders outer frame with expected size', () => {
  const html = render(
    React.createElement(SkyrimFrame, {
      width: 512,
      height: 704,
      header: true,
    }),
  );

  assert.equal(html.includes('class="frame"'), true);
  assert.equal(html.includes('width:512px'), true);
  assert.equal(html.includes('height:704px'), true);
  assert.equal(html.includes('Header-left'), true);
  assert.equal(html.includes('Border-middle'), true);
});

console.log('Unit tests passed');
