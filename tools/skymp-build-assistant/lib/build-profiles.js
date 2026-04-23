import fs from 'fs';
import path from 'path';
import { spawnSync } from 'child_process';

import { dashboardUiIndexPath, packageRoot } from './paths.js';

const commandAvailabilityCache = new Map();

const CMAKE_OPTION_DEFINITIONS = {
  BUILD_CLIENT: {
    key: 'BUILD_CLIENT',
    label: 'Build client',
    description: 'Include the SkyMP client targets in the configured CMake build.',
    defaultValue: true,
  },
  BUILD_FRONT: {
    key: 'BUILD_FRONT',
    label: 'Build front from local sources',
    description: 'Include the local skymp5-front CMake target in the configured build.',
    defaultValue: true,
  },
  BUILD_GAMEMODE: {
    key: 'BUILD_GAMEMODE',
    label: 'Build gamemode',
    description: 'Include the skymp5-functions-lib / gamemode targets in the configured build.',
    defaultValue: true,
  },
  BUILD_GAMEMODE_FROM_REMOTE: {
    key: 'BUILD_GAMEMODE_FROM_REMOTE',
    label: 'Use remote gamemode sources',
    description:
      'Pull gamemode sources from the remote repository instead of the local skymp5-gamemode checkout.',
    defaultValue: false,
  },
  BUILD_SKYRIM_PLATFORM: {
    key: 'BUILD_SKYRIM_PLATFORM',
    label: 'Build Skyrim Platform',
    description: 'Include the Skyrim Platform target and runtime packaging in the configured build.',
    defaultValue: true,
  },
  BUILD_NIRNLAB_RUNTIME: {
    key: 'BUILD_NIRNLAB_RUNTIME',
    label: 'Build NirnLab runtime',
    description: 'Include the optional NirnLab runtime target in the configured build.',
    defaultValue: false,
  },
  BUILD_SCRIPTS: {
    key: 'BUILD_SCRIPTS',
    label: 'Build scripts',
    description: 'Include skymp5-scripts and related Papyrus script targets in the configured build.',
    defaultValue: true,
  },
  BUILD_UNIT_TESTS: {
    key: 'BUILD_UNIT_TESTS',
    label: 'Build unit tests',
    description: 'Generate unit test binaries so CTest can run them.',
    defaultValue: true,
  },
};

function quoteShellPath(value) {
  return `"${String(value).replace(/"/g, '""')}"`;
}

function readCMakeCache(buildDir) {
  const cachePath = path.join(buildDir, 'CMakeCache.txt');
  if (!fs.existsSync(cachePath)) {
    return null;
  }

  const lines = fs.readFileSync(cachePath, 'utf8').split(/\r?\n/);
  const values = {};
  for (const line of lines) {
    if (!line || line.startsWith('#') || line.startsWith('//')) {
      continue;
    }
    const match = line.match(/^([^:]+):[^=]+=(.*)$/);
    if (!match) {
      continue;
    }
    values[match[1]] = match[2];
  }
  return values;
}

function readDeclaredCmakeOptions(repoRoot) {
  const sourcePath = path.join(repoRoot, 'CMakeLists.txt');
  if (!fs.existsSync(sourcePath)) {
    return new Set();
  }

  const declaredOptions = new Set();
  const lines = fs.readFileSync(sourcePath, 'utf8').split(/\r?\n/);
  for (const line of lines) {
    const match = line.match(/^\s*option\(\s*([A-Za-z0-9_]+)/i);
    if (match) {
      declaredOptions.add(match[1]);
    }
  }

  return declaredOptions;
}

function createRequirement(id, label, satisfied, detail, optional = false) {
  return {
    id,
    label,
    satisfied,
    detail,
    optional,
  };
}

function isTruthyCacheValue(value) {
  return String(value ?? '').toUpperCase() === 'ON';
}

function hasNodeModules(directory) {
  return fs.existsSync(path.join(directory, 'node_modules'));
}

function hasOwn(object, key) {
  return Object.prototype.hasOwnProperty.call(object ?? {}, key);
}

function isGamemodeRemoteBuild(state) {
  return isTruthyCacheValue(state.cmakeCache?.BUILD_GAMEMODE_FROM_REMOTE);
}

function hasStandaloneVoipPackage(state) {
  return fs.existsSync(path.join(state.voipDir, 'package.json'));
}

function commandExists(command) {
  if (commandAvailabilityCache.has(command)) {
    return commandAvailabilityCache.get(command);
  }

  let available = false;
  try {
    if (process.platform === 'win32') {
      const result = spawnSync(
        'powershell.exe',
        [
          '-NoProfile',
          '-Command',
          `Get-Command ${command} -ErrorAction SilentlyContinue | Select-Object -ExpandProperty Source`,
        ],
        {
          encoding: 'utf8',
          stdio: ['ignore', 'pipe', 'ignore'],
        },
      );
      available = result.status === 0 && Boolean(result.stdout?.trim());
    } else {
      const result = spawnSync('/bin/bash', ['-lc', `command -v ${command}`], {
        encoding: 'utf8',
        stdio: ['ignore', 'pipe', 'ignore'],
      });
      available = result.status === 0 && Boolean(result.stdout?.trim());
    }
  } catch {
    available = false;
  }

  commandAvailabilityCache.set(command, available);
  return available;
}

function buildState(runtime) {
  const repoRoot = runtime.repoRoot;
  const ctx = runtime.ctx;
  const buildDir = ctx.buildDir;
  const cmakeCache = readCMakeCache(buildDir);
  const declaredCmakeOptions = readDeclaredCmakeOptions(repoRoot);
  const cmakeConfigured = cmakeCache !== null;
  const currentPackageRoot = packageRoot();
  const frontDir = path.join(repoRoot, 'skymp5-front');
  const gamemodeDir = path.join(repoRoot, 'skymp5-gamemode');
  const functionsLibDir = path.join(repoRoot, 'skymp5-functions-lib');
  const voipDir = path.join(repoRoot, 'skymp5-voip');

  return {
    ...runtime,
    buildDir,
    cmakeCache,
    declaredCmakeOptions,
    cmakeConfigured,
    currentPackageRoot,
    frontDir,
    gamemodeDir,
    functionsLibDir,
    voipDir,
  };
}

function configureCommand(state) {
  return configureCommandWithArgs(state, []);
}

function configureCommandWithArgs(state, extraArgs = []) {
  const parts = [
    `cmake -S ${quoteShellPath(state.repoRoot)} -B ${quoteShellPath(state.buildDir)}`,
  ];
  if (state.ctx.skyrimRoot) {
    parts.push(`-DSKYRIM_DIR=${quoteShellPath(state.ctx.skyrimRoot)}`);
  }
  parts.push(...extraArgs);
  return parts.join(' ');
}

function cmakeBuildCommand(state, target = null) {
  const parts = ['cmake --build .', '--config Release'];
  if (target) {
    parts.push(`--target ${target}`);
  }
  return parts.join(' ');
}

function isPathInsideDirectory(directoryPath, candidatePath) {
  const relative = path.relative(path.resolve(directoryPath), path.resolve(candidatePath));
  return relative !== '' && !relative.startsWith('..') && !path.isAbsolute(relative);
}

function canSafelyCleanBuildDir(state) {
  if (!state.buildDir || !String(state.buildDir).trim()) {
    return false;
  }

  const resolvedBuildDir = path.resolve(state.buildDir);
  const parsedBuildDir = path.parse(resolvedBuildDir);
  if (resolvedBuildDir === parsedBuildDir.root) {
    return false;
  }

  return isPathInsideDirectory(state.repoRoot, resolvedBuildDir);
}

function canSafelyResetCmakeCache(state) {
  if (!state.buildDir || !String(state.buildDir).trim()) {
    return false;
  }

  const resolvedBuildDir = path.resolve(state.buildDir);
  const parsedBuildDir = path.parse(resolvedBuildDir);
  return resolvedBuildDir !== parsedBuildDir.root;
}

function resetCmakeCacheTargets(state) {
  return [
    path.join(state.buildDir, 'CMakeCache.txt'),
    path.join(state.buildDir, 'CMakeFiles'),
    path.join(state.buildDir, 'cmake_install.cmake'),
    path.join(state.buildDir, 'CTestTestfile.cmake'),
    path.join(state.buildDir, 'compile_commands.json'),
    path.join(state.buildDir, 'build.ninja'),
    path.join(state.buildDir, 'rules.ninja'),
    path.join(state.buildDir, '.ninja_deps'),
    path.join(state.buildDir, '.ninja_log'),
    path.join(state.buildDir, '.cmake'),
    path.join(state.buildDir, 'Makefile'),
    path.join(state.buildDir, 'skymp.sln'),
    path.join(state.buildDir, 'ALL_BUILD.vcxproj'),
    path.join(state.buildDir, 'ALL_BUILD.vcxproj.filters'),
    path.join(state.buildDir, 'RUN_TESTS.vcxproj'),
    path.join(state.buildDir, 'RUN_TESTS.vcxproj.filters'),
    path.join(state.buildDir, 'ZERO_CHECK.vcxproj'),
    path.join(state.buildDir, 'ZERO_CHECK.vcxproj.filters'),
  ];
}

function resetCmakeCacheCommand(state) {
  const quotedTargets = resetCmakeCacheTargets(state).map(quoteShellPath);

  if (process.platform === 'win32') {
    return [
      '$removed = 0',
      `$targets = @(${quotedTargets.join(', ')})`,
      'foreach ($target in $targets) {',
      '  if (Test-Path -LiteralPath $target) {',
      '    Remove-Item -LiteralPath $target -Recurse -Force',
      '    Write-Host "Removed CMake state: $target"',
      '    $removed += 1',
      '  }',
      '}',
      `if ($removed -eq 0) { Write-Host "No CMake configure state found under ${state.buildDir}" }`,
      `else { Write-Host "Reset CMake configure state in ${state.buildDir}" }`,
    ].join('; ');
  }

  return [
    'removed=0;',
    `for target in ${quotedTargets.join(' ')}; do`,
    '  if [ -e "$target" ]; then',
    '    rm -rf "$target";',
    '    echo "Removed CMake state: $target";',
    '    removed=$((removed + 1));',
    '  fi;',
    'done;',
    `if [ "$removed" -eq 0 ]; then echo "No CMake configure state found under ${state.buildDir}";`,
    `else echo "Reset CMake configure state in ${state.buildDir}"; fi`,
  ].join(' ');
}

function cleanBuildDirCommand(state) {
  const quotedBuildDir = quoteShellPath(state.buildDir);
  if (process.platform === 'win32') {
    return `if (Test-Path -LiteralPath ${quotedBuildDir}) { Remove-Item -LiteralPath ${quotedBuildDir} -Recurse -Force; Write-Host "Removed build directory: ${state.buildDir}" } else { Write-Host "Build directory not present: ${state.buildDir}" }`;
  }

  return [
    `if [ -d ${quotedBuildDir} ]; then`,
    `  rm -rf ${quotedBuildDir};`,
    `  echo "Removed build directory: ${state.buildDir}";`,
    'else',
    `  echo "Build directory not present: ${state.buildDir}";`,
    'fi',
  ].join(' ');
}

function cleanBuildDirRequirements(state) {
  return [
    createRequirement(
      'build-dir-configured',
      'Configured build directory',
      Boolean(state.buildDir && String(state.buildDir).trim()),
      'Configure buildDir before using clean-build-dir.',
    ),
    createRequirement(
      'build-dir-safe-to-delete',
      'Build directory is inside repo',
      canSafelyCleanBuildDir(state),
      `Refusing to delete ${state.buildDir}. clean-build-dir only removes build directories inside the repo.`,
    ),
  ];
}

function resetCmakeCacheRequirements(state) {
  return [
    createRequirement(
      'build-dir-configured',
      'Configured build directory',
      Boolean(state.buildDir && String(state.buildDir).trim()),
      'Configure buildDir before resetting the CMake cache.',
    ),
    createRequirement(
      'build-dir-safe-to-reset',
      'Build directory is not a filesystem root',
      canSafelyResetCmakeCache(state),
      `Refusing to remove CMake state from ${state.buildDir}. Reset CMake cache only targets non-root build directories.`,
    ),
  ];
}

function buildAllRequirements(state) {
  const requirements = [
    createRequirement(
      'cmake-command',
      'cmake on PATH',
      commandExists('cmake'),
      'cmake is not available on PATH for the dashboard/CLI process.',
    ),
    createRequirement(
      'vcpkg-toolchain',
      'vcpkg submodule',
      fs.existsSync(path.join(state.repoRoot, 'vcpkg', 'scripts', 'buildsystems', 'vcpkg.cmake')),
      'The vcpkg submodule is missing. Run git submodule init && git submodule update first.',
    ),
    createRequirement(
      'front-package',
      'skymp5-front package',
      fs.existsSync(path.join(state.frontDir, 'package.json')),
      'skymp5-front/package.json is missing.',
    ),
    createRequirement(
      'front-node-modules',
      'Installed frontend dependencies',
      hasNodeModules(state.frontDir),
      'Install dependencies in skymp5-front first (npm install or yarn install).',
    ),
  ];

  if (hasStandaloneVoipPackage(state)) {
    requirements.push(
      createRequirement(
        'voip-node-modules',
        'Installed VoIP dependencies',
        hasNodeModules(state.voipDir),
        'Install dependencies in skymp5-voip first (npm install or yarn install).',
      ),
    );
  }

  return requirements;
}

function buildAllDependencies(state) {
  return hasStandaloneVoipPackage(state)
    ? ['configure-cmake', 'cmake-default', 'front-in-tree-build', 'voip-build']
    : ['configure-cmake', 'cmake-default', 'front-in-tree-build'];
}

function buildAllSteps(state) {
  const steps = [
    {
      id: 'configure',
      label: 'Run cmake configure',
      cwd: state.repoRoot,
      command: configureCommand(state),
    },
    {
      id: 'build-default',
      label: 'Build full default solution',
      cwd: state.buildDir,
      command: cmakeBuildCommand(state),
    },
    {
      id: 'front-build',
      label: 'Run skymp5-front build',
      cwd: state.frontDir,
      command: 'npm run build',
    },
  ];

  if (hasStandaloneVoipPackage(state)) {
    steps.push({
      id: 'voip-build',
      label: 'Run skymp5-voip build',
      cwd: state.voipDir,
      command: 'npm run build',
    });
  }

  return steps;
}

function buildAllExpectedOutputs(state) {
  const outputs = [
    {
      path: path.join(state.buildDir, 'CMakeCache.txt'),
      label: 'CMake cache',
    },
    {
      path: path.join(state.buildDir, 'dist', 'client'),
      label: 'Client dist directory',
    },
    {
      path: path.join(state.buildDir, 'dist', 'server'),
      label: 'Server dist directory',
    },
    {
      path: path.join(state.buildDir, 'dist', 'client', 'Data', 'Platform', 'UI'),
      label: 'Built front UI output',
    },
  ];

  if (hasStandaloneVoipPackage(state)) {
    outputs.push(
      {
        path: path.join(
          state.buildDir,
          'dist',
          'client',
          'Data',
          'Platform',
          'Plugins',
          'skymp5-voip.js',
        ),
        label: 'VoIP client plugin',
      },
      {
        path: path.join(state.buildDir, 'dist', 'voip', 'voip-dev-server.js'),
        label: 'VoIP dev server bundle',
      },
    );
  }

  return outputs;
}

function getCmakeOptionDefinition(optionKey) {
  return CMAKE_OPTION_DEFINITIONS[optionKey] ?? null;
}

function isDeclaredCmakeOption(state, optionKey) {
  return state.declaredCmakeOptions?.has(optionKey) === true;
}

function createCmakeOptionRequirement(
  state,
  optionKey,
  {
    id,
    unsupportedLabel = `${optionKey} supported by this checkout`,
    unsupportedDetail = `This checkout does not define ${optionKey} in CMake.`,
    enabledDetail = `This profile requires CMake to be configured with ${optionKey}=ON.`,
  } = {},
) {
  const declared = isDeclaredCmakeOption(state, optionKey);
  return createRequirement(
    id ?? optionKey.toLowerCase(),
    declared ? `${optionKey}=ON` : unsupportedLabel,
    declared ? isTruthyCacheValue(state.cmakeCache?.[optionKey]) : false,
    declared ? enabledDetail : unsupportedDetail,
  );
}

function normalizeCmakeOptionRefs(optionRefs) {
  return (optionRefs ?? [])
    .map((optionRef) => (typeof optionRef === 'string' ? { key: optionRef } : optionRef))
    .filter((optionRef) => getCmakeOptionDefinition(optionRef.key));
}

function resolveCmakeOptionValue(state, optionKey) {
  if (!isDeclaredCmakeOption(state, optionKey)) {
    return null;
  }

  if (state.cmakeConfigured && hasOwn(state.cmakeCache, optionKey)) {
    return isTruthyCacheValue(state.cmakeCache?.[optionKey]);
  }

  return getCmakeOptionDefinition(optionKey)?.defaultValue ?? false;
}

function buildProfileCmakeOptions(state, optionRefs) {
  const cmakeAvailable = commandExists('cmake');

  return normalizeCmakeOptionRefs(optionRefs).map((optionRef) => {
    const definition = getCmakeOptionDefinition(optionRef.key);
    const declared = isDeclaredCmakeOption(state, definition.key);
    const presentInCache = hasOwn(state.cmakeCache, definition.key);
    return {
      key: definition.key,
      label: optionRef.label ?? definition.label,
      description: optionRef.description ?? definition.description,
      value: resolveCmakeOptionValue(state, definition.key),
      defaultValue: definition.defaultValue,
      supported: declared,
      source:
        !declared && presentInCache
          ? 'stale-cache'
          : declared && state.cmakeConfigured && presentInCache
            ? 'cache'
            : declared
              ? 'default'
              : 'unsupported',
      toggleable: cmakeAvailable && declared,
      disabledReason: !declared
        ? `This checkout does not define ${definition.key} in CMake. The cache entry may be stale from an older configure.`
        : cmakeAvailable
          ? ''
          : 'cmake is not available on PATH for the dashboard/CLI process.',
    };
  });
}

function runShellCommand(command, cwd) {
  if (process.platform === 'win32') {
    return spawnSync('powershell.exe', ['-NoProfile', '-Command', command], {
      cwd,
      encoding: 'utf8',
      stdio: ['ignore', 'pipe', 'pipe'],
      windowsHide: true,
    });
  }

  return spawnSync('/bin/bash', ['-lc', command], {
    cwd,
    encoding: 'utf8',
    stdio: ['ignore', 'pipe', 'pipe'],
  });
}

function finalizeProfile(base, runtime) {
  const state = buildState(runtime);
  const requirements = base.requirements ? base.requirements(state) : [];
  const failedRequirement = requirements.find((requirement) => !requirement.satisfied && !requirement.optional);
  const enabled = !failedRequirement;
  const disabledReason = failedRequirement ? failedRequirement.detail : null;
  const dependencies =
    typeof base.dependencies === 'function' ? base.dependencies(state) : (base.dependencies ?? []);

  return {
    id: base.id,
    label: base.label,
    description: base.description,
    category: base.category,
    executionMode: base.executionMode ?? 'one-shot',
    longRunningCapable: base.longRunningCapable ?? false,
    dependencies,
    shell: base.shell ?? 'powershell',
    destructive: base.destructive ?? false,
    confirmMessage: base.confirmMessage ? base.confirmMessage(state) : null,
    requirements,
    enabled,
    disabledReason,
    steps: base.steps(state),
    expectedOutputs: base.expectedOutputs ? base.expectedOutputs(state) : [],
    cmakeOptions: buildProfileCmakeOptions(state, base.cmakeOptions),
  };
}

const PROFILE_DEFINITIONS = [
  {
    id: 'configure-cmake',
    label: 'Configure CMake',
    description:
      'Generate or update the fixed SkyMP build tree using the current configured paths.',
    category: 'cmake',
    dependencies: [],
    requirements: (state) => [
      createRequirement(
        'cmake-command',
        'cmake on PATH',
        commandExists('cmake'),
        'cmake is not available on PATH for the dashboard/CLI process.',
      ),
      createRequirement(
        'vcpkg-toolchain',
        'vcpkg submodule',
        fs.existsSync(path.join(state.repoRoot, 'vcpkg', 'scripts', 'buildsystems', 'vcpkg.cmake')),
        'The vcpkg submodule is missing. Run git submodule init && git submodule update first.',
      ),
    ],
    steps: (state) => [
      {
        id: 'configure',
        label: 'Run cmake configure',
        cwd: state.repoRoot,
        command: configureCommand(state),
      },
    ],
    expectedOutputs: (state) => [
      {
        path: path.join(state.buildDir, 'CMakeCache.txt'),
        label: 'CMake cache',
      },
    ],
  },
  {
    id: 'reset-cmake-cache',
    label: 'Reset CMake cache',
    description:
      'Remove CMakeCache.txt, CMakeFiles, and top-level generated buildsystem files while leaving compiled outputs and vcpkg state intact.',
    category: 'cmake',
    dependencies: [],
    destructive: true,
    confirmMessage: (state) =>
      `This removes CMake configure state from ${state.buildDir} and requires configure-cmake to be run again before the next CMake build.`,
    requirements: resetCmakeCacheRequirements,
    steps: (state) => [
      {
        id: 'reset-cmake-cache',
        label: 'Remove CMake configure state',
        cwd: state.repoRoot,
        command: resetCmakeCacheCommand(state),
      },
    ],
    expectedOutputs: () => [],
  },
  {
    id: 'build-all',
    label: 'Build all core outputs',
    description:
      'Configure CMake, build the default solution, then refresh the local front and VoIP outputs into the configured build directory.',
    category: 'workflow',
    dependencies: buildAllDependencies,
    requirements: buildAllRequirements,
    steps: buildAllSteps,
    expectedOutputs: buildAllExpectedOutputs,
  },
  {
    id: 'rebuild-all',
    label: 'Clean and rebuild all',
    description:
      'Delete the configured build directory, reconfigure CMake, and rebuild the core outputs from scratch.',
    category: 'workflow',
    destructive: true,
    confirmMessage: (state) =>
      `This deletes ${state.buildDir} before rebuilding the core outputs from scratch.`,
    dependencies: (state) => ['clean-build-dir', ...buildAllDependencies(state)],
    requirements: (state) => [...cleanBuildDirRequirements(state), ...buildAllRequirements(state)],
    steps: (state) => [
      {
        id: 'clean-build-dir',
        label: 'Delete build directory',
        cwd: state.repoRoot,
        command: cleanBuildDirCommand(state),
      },
      ...buildAllSteps(state),
    ],
    expectedOutputs: buildAllExpectedOutputs,
  },
  {
    id: 'clean-build-dir',
    label: 'Clean build directory',
    description: 'Delete the configured build directory without rebuilding anything.',
    category: 'workflow',
    destructive: true,
    confirmMessage: (state) =>
      `This deletes ${state.buildDir}. Use rebuild-all if you want to clean and immediately rebuild.`,
    dependencies: [],
    requirements: cleanBuildDirRequirements,
    steps: (state) => [
      {
        id: 'clean-build-dir',
        label: 'Delete build directory',
        cwd: state.repoRoot,
        command: cleanBuildDirCommand(state),
      },
    ],
    expectedOutputs: () => [],
  },
  {
    id: 'cmake-default',
    label: 'CMake default build',
    description: 'Build the current CMake configuration with Release outputs.',
    category: 'cmake',
    dependencies: ['configure-cmake'],
    requirements: (state) => [
      createRequirement(
        'cmake-command',
        'cmake on PATH',
        commandExists('cmake'),
        'cmake is not available on PATH for the dashboard/CLI process.',
      ),
      createRequirement(
        'cmake-configured',
        'Configured build tree',
        state.cmakeConfigured,
        'Run the configure-cmake profile first so the build directory contains CMakeCache.txt.',
      ),
    ],
    steps: (state) => [
      {
        id: 'build-default',
        label: 'Build full default solution',
        cwd: state.buildDir,
        command: cmakeBuildCommand(state),
      },
    ],
    expectedOutputs: (state) => [
      {
        path: path.join(state.buildDir, 'dist', 'client'),
        label: 'Client dist directory',
      },
      {
        path: path.join(state.buildDir, 'dist', 'server'),
        label: 'Server dist directory',
      },
    ],
    cmakeOptions: ['BUILD_CLIENT', 'BUILD_FRONT', 'BUILD_GAMEMODE', 'BUILD_SKYRIM_PLATFORM', 'BUILD_SCRIPTS'],
  },
  {
    id: 'cmake-client-only',
    label: 'CMake client build',
    description: 'Build the SkyMP client plugin through the configured CMake target.',
    category: 'cmake',
    dependencies: ['configure-cmake'],
    requirements: (state) => [
      createRequirement(
        'cmake-command',
        'cmake on PATH',
        commandExists('cmake'),
        'cmake is not available on PATH for the dashboard/CLI process.',
      ),
      createRequirement(
        'cmake-configured',
        'Configured build tree',
        state.cmakeConfigured,
        'Run the configure-cmake profile first so the build directory contains CMakeCache.txt.',
      ),
    ],
    steps: (state) => [
      {
        id: 'build-client',
        label: 'Build skymp5-client target',
        cwd: state.buildDir,
        command: cmakeBuildCommand(state, 'skymp5-client'),
      },
    ],
    expectedOutputs: (state) => [
      {
        path: path.join(
          state.buildDir,
          'dist',
          'client',
          'Data',
          'Platform',
          'Plugins',
          'skymp5-client.js',
        ),
        label: 'skymp5-client.js',
      },
    ],
    cmakeOptions: ['BUILD_CLIENT'],
  },
  {
    id: 'cmake-server-only',
    label: 'CMake server build',
    description: 'Build the SkyMP server target and bundled server JavaScript output.',
    category: 'cmake',
    dependencies: ['configure-cmake'],
    requirements: (state) => [
      createRequirement(
        'cmake-command',
        'cmake on PATH',
        commandExists('cmake'),
        'cmake is not available on PATH for the dashboard/CLI process.',
      ),
      createRequirement(
        'cmake-configured',
        'Configured build tree',
        state.cmakeConfigured,
        'Run the configure-cmake profile first so the build directory contains CMakeCache.txt.',
      ),
    ],
    steps: (state) => [
      {
        id: 'build-server',
        label: 'Build skymp5-server target',
        cwd: state.buildDir,
        command: cmakeBuildCommand(state, 'skymp5-server'),
      },
    ],
    expectedOutputs: (state) => [
      {
        path: path.join(state.buildDir, 'dist', 'server', 'dist_back', 'skymp5-server.js'),
        label: 'Server bundle',
      },
    ],
  },
  {
    id: 'cmake-platform-only',
    label: 'CMake Skyrim Platform build',
    description: 'Build Skyrim Platform and pack its runtime into build/dist/client.',
    category: 'cmake',
    dependencies: ['configure-cmake'],
    requirements: (state) => [
      createRequirement(
        'cmake-command',
        'cmake on PATH',
        commandExists('cmake'),
        'cmake is not available on PATH for the dashboard/CLI process.',
      ),
      createRequirement(
        'cmake-configured',
        'Configured build tree',
        state.cmakeConfigured,
        'Run the configure-cmake profile first so the build directory contains CMakeCache.txt.',
      ),
    ],
    steps: (state) => [
      {
        id: 'build-platform',
        label: 'Build skyrim-platform target',
        cwd: state.buildDir,
        command: cmakeBuildCommand(state, 'skyrim-platform'),
      },
    ],
    expectedOutputs: (state) => [
      {
        path: path.join(
          state.buildDir,
          'dist',
          'client',
          'Data',
          'SKSE',
          'Plugins',
          'SkyrimPlatform.dll',
        ),
        label: 'SkyrimPlatform.dll',
      },
    ],
    cmakeOptions: ['BUILD_SKYRIM_PLATFORM'],
  },
  {
    id: 'front-in-tree-build',
    label: 'Front in-tree build',
    description: 'Run the local skymp5-front webpack build directly without CMake.',
    category: 'node',
    dependencies: [],
    requirements: (state) => [
      createRequirement(
        'front-package',
        'skymp5-front package',
        fs.existsSync(path.join(state.frontDir, 'package.json')),
        'skymp5-front/package.json is missing.',
      ),
      createRequirement(
        'front-node-modules',
        'Installed frontend dependencies',
        hasNodeModules(state.frontDir),
        'Install dependencies in skymp5-front first (npm install or yarn install).',
      ),
    ],
    steps: (state) => [
      {
        id: 'front-build',
        label: 'Run skymp5-front build',
        cwd: state.frontDir,
        command: 'npm run build',
      },
    ],
    expectedOutputs: (state) => [
      {
        path: path.join(state.buildDir, 'dist', 'client', 'Data', 'Platform', 'UI'),
        label: 'Built front UI output',
      },
    ],
  },
  {
    id: 'voip-build',
    label: 'VoIP build',
    description: 'Run the standalone VoIP build that writes into build/dist.',
    category: 'node',
    dependencies: [],
    requirements: (state) => [
      createRequirement(
        'voip-package',
        'skymp5-voip package',
        fs.existsSync(path.join(state.voipDir, 'package.json')),
        'skymp5-voip/package.json is missing.',
      ),
      createRequirement(
        'voip-node-modules',
        'Installed VoIP dependencies',
        hasNodeModules(state.voipDir),
        'Install dependencies in skymp5-voip first (npm install or yarn install).',
      ),
    ],
    steps: (state) => [
      {
        id: 'voip-build',
        label: 'Run skymp5-voip build',
        cwd: state.voipDir,
        command: 'npm run build',
      },
    ],
    expectedOutputs: (state) => [
      {
        path: path.join(
          state.buildDir,
          'dist',
          'client',
          'Data',
          'Platform',
          'Plugins',
          'skymp5-voip.js',
        ),
        label: 'VoIP client plugin',
      },
      {
        path: path.join(state.buildDir, 'dist', 'voip', 'voip-dev-server.js'),
        label: 'VoIP dev server bundle',
      },
    ],
  },
  {
    id: 'build-assistant-ui',
    label: 'Build Assistant UI',
    description: 'Build the build-assistant dashboard frontend bundle.',
    category: 'assistant',
    dependencies: [],
    requirements: (state) => [
      createRequirement(
        'assistant-node-modules',
        'Installed build assistant dependencies',
        hasNodeModules(state.currentPackageRoot),
        'Run npm install in tools/skymp-build-assistant first.',
      ),
    ],
    steps: (state) => [
      {
        id: 'assistant-ui-build',
        label: 'Run build-assistant UI build',
        cwd: state.currentPackageRoot,
        command: 'npm run build:ui',
      },
    ],
    expectedOutputs: () => [
      {
        path: dashboardUiIndexPath(),
        label: 'Built dashboard index.html',
      },
    ],
  },
  {
    id: 'cmake-front-local',
    label: 'CMake local front build',
    description: 'Build the local skymp5-front CMake target when BUILD_FRONT is enabled.',
    category: 'advanced',
    dependencies: ['configure-cmake'],
    requirements: (state) => [
      createRequirement(
        'cmake-command',
        'cmake on PATH',
        commandExists('cmake'),
        'cmake is not available on PATH for the dashboard/CLI process.',
      ),
      createRequirement(
        'cmake-configured',
        'Configured build tree',
        state.cmakeConfigured,
        'Run the configure-cmake profile first so the build directory contains CMakeCache.txt.',
      ),
      createCmakeOptionRequirement(state, 'BUILD_FRONT', {
        id: 'build-front-on',
      }),
      createRequirement(
        'cmake-front-local-script',
        'Local front build script',
        fs.existsSync(path.join(state.frontDir, 'build-local.cmake')),
        'skymp5-front/build-local.cmake is missing. Update this branch to the local-only front build first.',
      ),
      createRequirement(
        'cmake-front-package',
        'Local skymp5-front checkout',
        fs.existsSync(path.join(state.frontDir, 'package.json')),
        'skymp5-front/package.json is missing. The CMake front build now uses the local checkout.',
      ),
    ],
    steps: (state) => [
      {
        id: 'build-front-target',
        label: 'Build skymp5-front target',
        cwd: state.buildDir,
        command: cmakeBuildCommand(state, 'skymp5-front'),
      },
    ],
    expectedOutputs: (state) => [
      {
        path: path.join(state.buildDir, 'dist', 'client', 'Data', 'Platform', 'UI'),
        label: 'Front UI output',
      },
    ],
    cmakeOptions: ['BUILD_FRONT'],
  },
  {
    id: 'cmake-gamemode',
    label: 'CMake gamemode build',
    description:
      'Build the gamemode/functions target when BUILD_GAMEMODE is enabled. Local skymp5-gamemode sources are used by default; remote mode additionally requires BUILD_GAMEMODE_FROM_REMOTE=ON and GITHUB_TOKEN.',
    category: 'advanced',
    dependencies: ['configure-cmake'],
    requirements: (state) => [
      createRequirement(
        'cmake-command',
        'cmake on PATH',
        commandExists('cmake'),
        'cmake is not available on PATH for the dashboard/CLI process.',
      ),
      createRequirement(
        'cmake-configured',
        'Configured build tree',
        state.cmakeConfigured,
        'Run the configure-cmake profile first so the build directory contains CMakeCache.txt.',
      ),
      createCmakeOptionRequirement(state, 'BUILD_GAMEMODE', {
        id: 'build-gamemode-on',
      }),
      createRequirement(
        'gamemode-local-build-script',
        'Local gamemode build script',
        isGamemodeRemoteBuild(state) ||
          fs.existsSync(path.join(state.functionsLibDir, 'build-local.cmake')),
        'skymp5-functions-lib/build-local.cmake is missing. Update this branch to the local-default gamemode build first.',
      ),
      createRequirement(
        'gamemode-local-source',
        'Local skymp5-gamemode entry file',
        isGamemodeRemoteBuild(state) ||
          fs.existsSync(path.join(state.gamemodeDir, 'gamemode.js')),
        'skymp5-gamemode/gamemode.js is missing. The default gamemode build now uses the local checkout.',
      ),
      createRequirement(
        'github-token',
        'GITHUB_TOKEN for remote gamemode mode',
        !isGamemodeRemoteBuild(state) || Boolean(process.env.GITHUB_TOKEN),
        'Set GITHUB_TOKEN before running the gamemode build in BUILD_GAMEMODE_FROM_REMOTE mode.',
      ),
    ],
    steps: (state) => [
      {
        id: 'build-gamemode',
        label: 'Build skymp5-functions-lib target',
        cwd: state.buildDir,
        command: cmakeBuildCommand(state, 'skymp5-functions-lib'),
      },
    ],
    expectedOutputs: (state) => [
      {
        path: path.join(state.buildDir, 'dist', 'server', 'gamemode.js'),
        label: 'gamemode.js (server dist)',
      },
      ...(
        isGamemodeRemoteBuild(state)
          ? []
          : [
              {
                path: path.join(
                  state.buildDir,
                  'dist',
                  'server',
                  'skymp5-gamemode',
                  'gamemode.js',
                ),
                label: 'gamemode.js (nested local dist path)',
              },
            ]
      ),
    ],
    cmakeOptions: ['BUILD_GAMEMODE', 'BUILD_GAMEMODE_FROM_REMOTE'],
  },
  {
    id: 'cmake-nirnlab',
    label: 'CMake NirnLab build',
    description: 'Build the optional NirnLab runtime target when enabled in CMake.',
    category: 'advanced',
    dependencies: ['configure-cmake'],
    requirements: (state) => [
      createRequirement(
        'cmake-command',
        'cmake on PATH',
        commandExists('cmake'),
        'cmake is not available on PATH for the dashboard/CLI process.',
      ),
      createRequirement(
        'cmake-configured',
        'Configured build tree',
        state.cmakeConfigured,
        'Run the configure-cmake profile first so the build directory contains CMakeCache.txt.',
      ),
      createCmakeOptionRequirement(state, 'BUILD_NIRNLAB_RUNTIME', {
        id: 'build-nirnlab-on',
        unsupportedDetail:
          'This checkout does not define BUILD_NIRNLAB_RUNTIME in the main CMake project. The cache entry is stale or from an older configure.',
      }),
    ],
    steps: (state) => [
      {
        id: 'build-nirnlab',
        label: 'Build nirnlab-runtime target',
        cwd: state.buildDir,
        command: cmakeBuildCommand(state, 'nirnlab-runtime'),
      },
    ],
    expectedOutputs: (state) => [
      {
        path: path.join(state.buildDir, 'dist', 'client', 'Data', 'NirnLabUIPlatform'),
        label: 'NirnLab runtime output',
      },
    ],
    cmakeOptions: ['BUILD_NIRNLAB_RUNTIME'],
  },
  {
    id: 'ctest',
    label: 'CTest',
    description: 'Run the configured test suite from the fixed build directory.',
    category: 'advanced',
    dependencies: ['configure-cmake'],
    requirements: (state) => [
      createRequirement(
        'ctest-command',
        'ctest on PATH',
        commandExists('ctest'),
        'ctest is not available on PATH for the dashboard/CLI process.',
      ),
      createRequirement(
        'cmake-configured',
        'Configured build tree',
        state.cmakeConfigured,
        'Run the configure-cmake profile first so the build directory contains CMakeCache.txt.',
      ),
    ],
    steps: (state) => [
      {
        id: 'run-ctest',
        label: 'Run ctest --verbose',
        cwd: state.buildDir,
        command: 'ctest --verbose',
      },
    ],
    expectedOutputs: () => [],
    cmakeOptions: ['BUILD_UNIT_TESTS'],
  },
];

export function listBuildProfiles(runtime) {
  return PROFILE_DEFINITIONS.map((definition) => finalizeProfile(definition, runtime));
}

export function getBuildProfile(profileId, runtime) {
  return listBuildProfiles(runtime).find((profile) => profile.id === profileId) ?? null;
}

export function setBuildCmakeOption(optionKey, enabled, runtime) {
  const definition = getCmakeOptionDefinition(optionKey);
  if (!definition) {
    const error = new Error(`Unsupported CMake option: ${optionKey}`);
    error.code = 'BUILD_CMAKE_OPTION_UNSUPPORTED';
    throw error;
  }

  const state = buildState(runtime);
  if (!isDeclaredCmakeOption(state, definition.key)) {
    const error = new Error(
      `This checkout does not define ${definition.key} in CMake. The cache entry may be stale from an older configure.`,
    );
    error.code = 'BUILD_CMAKE_OPTION_UNSUPPORTED_BY_CHECKOUT';
    error.optionKey = definition.key;
    throw error;
  }

  if (!commandExists('cmake')) {
    const error = new Error('cmake is not available on PATH for the dashboard/CLI process.');
    error.code = 'BUILD_CMAKE_OPTION_UNAVAILABLE';
    throw error;
  }

  const command = configureCommandWithArgs(state, [
    `-D${definition.key}=${enabled === true ? 'ON' : 'OFF'}`,
  ]);
  const result = runShellCommand(command, state.repoRoot);

  if (result.status !== 0) {
    const output = [result.stdout, result.stderr]
      .map((chunk) => String(chunk || '').trim())
      .filter(Boolean)
      .join('\n');
    const error = new Error(
      output || `Failed to configure CMake with ${definition.key}=${enabled === true ? 'ON' : 'OFF'}.`,
    );
    error.code = 'BUILD_CMAKE_OPTION_CONFIGURE_FAILED';
    error.optionKey = definition.key;
    throw error;
  }

  return {
    option: {
      ...definition,
      value: enabled === true,
    },
    profiles: listBuildProfiles(runtime),
    command,
  };
}
