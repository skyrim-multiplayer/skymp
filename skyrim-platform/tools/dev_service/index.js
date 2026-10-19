let fs = require('fs-extra');
let path = require('path');
let game = require('./game');

// Keep this in sync with triplet file overlay_triplets\x64-windows-sp.cmake or similar
// Note: at this moment, this list is empty. it was "spdlog" and "fmt" before, but now we use static linking for them.
const requiredVcpkgDlls = [];

const requiredVcpkgDllsNodeJs = 'libnode.dll';

function writeFileSyncRecursive(filename, content, charset) {
  filename
    .split(path.sep)
    .slice(0, -1)
    .reduce((last, folder) => {
      let folderPath = last ? last + path.sep + folder : folder;
      if (!fs.existsSync(folderPath)) {
        fs.mkdirSync(folderPath);
      }
      return folderPath;
    });

  fs.writeFileSync(filename, content, charset);
}

if (!fs.existsSync('config.js')) {
  fs.writeFileSync('config.js', fs.readFileSync('config-default.js'));
}
let config = require('./config');

console.log('Dev service started');

const getBinaryDir = () => path.resolve(__dirname, '../../../build');
const getSourceDir = () => path.resolve(__dirname, '../..');

console.log(`Binary dir is '${getBinaryDir()}'`);
console.log(`Source dir is '${getSourceDir()}'`);

let bin = path.join(getBinaryDir(), 'skyrim-platform/_platform_se');
let sourceDir = path.join(getSourceDir());
let distDir = path.join(getBinaryDir(), 'dist/client');

const createDirectory = (path) => {
  if (!fs.existsSync(path)) {
    while (true)
      try {
        fs.mkdirSync(path, { recursive: true });
        break;
      } catch (e) {
        // Temporary EPERM errors encountered on Windows. They disappear if we retry.
        // With Linux, we don't want to experiment and give our CI a chance to stuck in an infinite loop
        if (
          process.platform !== 'win32' ||
          e.toString().indexOf('EPERM') === -1
        ) {
          throw e;
        }
      }
  }
};

const isTruthyEnv = (value) =>
  ['1', 'true', 'yes', 'on'].includes((value || '').toLowerCase());

const supportedBuildConfigs = ['Debug', 'Release', 'RelWithDebInfo', 'MinSizeRel'];

const normalizeBuildConfig = (value) => {
  const candidate = (value || '').trim();
  return supportedBuildConfigs.includes(candidate) ? candidate : null;
};

const watchCallback = (_eventType, fileName) => {
  {
    if (fileName && fileName.startsWith('touch_')) {
      let buildCfg = normalizeBuildConfig(fileName.slice('touch_'.length));
      if (!buildCfg) {
        return;
      }

      console.log('Skyrim Platform ' + buildCfg + ' x64 updated.');

      if (!process.env.DEV_SERVICE_NO_GAME) {
        console.log('Stopping Skyrim SE...');
        game.kill();
      }

      // All except "Plugins" since we need RestartGame target to keep Plugins directory alive in watch mode
      const directoriesToClear = [
        path.join(distDir, 'Data/Platform/Distribution'),
        path.join(distDir, 'Data/Platform/Modules'),
        path.join(distDir, 'Data/Platform/plugin-example'),
      ];
      directoriesToClear.forEach((directory) => {
        if (fs.existsSync(directory)) {
          fs.removeSync(directory);
        }
      });
      createDirectory(distDir);
      let getFileName = (p) => p.replace(/^.*[\\\/]/, '');
      let cp = (from, targetDir) =>
        writeFileSyncRecursive(
          path.join(targetDir, getFileName(from)),
          fs.readFileSync(from),
        );
      let cpIfExists = (from, targetDir) => {
        if (fs.existsSync(from)) {
          cp(from, targetDir);
        } else {
          console.warn(`Optional file is missing, skipping copy: ${from}`);
        }
      };
      let unlinkIfExists = (filePath) => {
        if (fs.existsSync(filePath)) {
          fs.unlinkSync(filePath);
        }
      };
      let binPath = (file) => path.join(bin, `bin/${buildCfg}/${file}`);

      if (process.platform === 'win32') {
        const cefDir = fs
          .readFileSync(path.join(bin, `cef_dir.txt`))
          .toString('utf-8')
          .trim();
        const cefRuntimeDirCandidates = [
          path.join(cefDir, buildCfg),
          path.join(cefDir, 'Release'),
          path.join(cefDir, 'RelWithDebInfo'),
          path.join(cefDir, 'MinSizeRel'),
          path.join(cefDir, 'Debug'),
        ];
        const cefRuntimeDir = cefRuntimeDirCandidates.find((dir) =>
          fs.existsSync(path.join(dir, 'chrome_elf.dll')),
        );
        const cefResourcesDir = path.join(cefDir, 'Resources');
        const canCopyCefAssets =
          !!cefRuntimeDir &&
          fs.existsSync(path.join(cefResourcesDir, 'icudtl.dat'));
        const isNoGameMode = isTruthyEnv(process.env.DEV_SERVICE_NO_GAME);

        if (!canCopyCefAssets) {
          const cefError =
            `Unable to locate required CEF runtime assets under '${cefDir}'. ` +
            `Checked runtime directories: ${cefRuntimeDirCandidates.join(', ')}; ` +
            `resources directory: ${cefResourcesDir}`;
          if (isNoGameMode) {
            console.warn(`${cefError}. Skipping CEF runtime copy in no-game mode.`);
          } else {
            throw new Error(cefError);
          }
        } else {
          [
            'chrome_elf.dll',
            'd3dcompiler_47.dll',
            'libcef.dll',
            'libEGL.dll',
            'libGLESv2.dll',
            'snapshot_blob.bin',
            'v8_context_snapshot.bin',
          ].forEach((item) => {
            cp(
              path.join(cefRuntimeDir, item),
              path.join(
                distDir,
                'Data/Platform/Distribution/RuntimeDependencies',
              ),
            );
          });
          ['icudtl.dat'].forEach((item) => {
            cp(
              path.join(cefResourcesDir, item),
              path.join(
                distDir,
                'Data/Platform/Distribution/RuntimeDependencies',
              ),
            );
          });
          [
            'chrome_100_percent.pak',
            'chrome_200_percent.pak',
            'resources.pak',
          ].forEach((item) => {
            cp(
              path.join(cefResourcesDir, item),
              path.join(distDir, 'Data/Platform/Distribution/CEF'),
            );
          });
          fs.copySync(
            path.join(cefResourcesDir, 'locales'),
            path.join(distDir, 'Data/Platform/Distribution/CEF/locales'),
          );
        }

        cpIfExists(binPath('SkyrimPlatform.pdb'), distDir);
        cpIfExists(binPath('SkyrimPlatformImpl.pdb'), distDir);
        requiredVcpkgDlls.forEach((dll) => {
          cp(
            binPath(dll),
            path.join(
              distDir,
              'Data/Platform/Distribution/RuntimeDependencies',
            ),
          );
        });
        const nodeEmbedderApiDirs = [
          'node-embedder-api',
          'node-embedder-api-prebuilt',
        ];
        let copiedNodeDlls = false;
        nodeEmbedderApiDirs.forEach((dirName) => {
          const binaryDir = getBinaryDir();
          const nodeEmbedderApiDir = path.join(
            binaryDir,
            `vcpkg_installed/x64-windows-sp/bin/${dirName}`,
          );
          if (!copiedNodeDlls) {
            console.log(
              `Copying NodeJS dll ${requiredVcpkgDllsNodeJs} from ${dirName} to dist`,
            );
            if (
              fs.existsSync(
                path.join(nodeEmbedderApiDir, requiredVcpkgDllsNodeJs),
              )
            ) {
              cp(
                path.join(nodeEmbedderApiDir, requiredVcpkgDllsNodeJs),
                path.join(
                  distDir,
                  'Data/Platform/Distribution/RuntimeDependencies',
                ),
              );
              copiedNodeDlls = true;
            }
          }
        });
        cp(
          binPath('SkyrimPlatformCEF.exe.hidden'),
          path.join(distDir, 'Data/Platform/Distribution/RuntimeDependencies'),
        );
        cpIfExists(binPath('SkyrimPlatformCEF.pdb'), distDir);
        cp(
          binPath('SkyrimPlatform.dll'),
          path.join(distDir, 'Data/SKSE/Plugins'),
        );
        cp(
          binPath('SkyrimPlatformImpl.dll'),
          path.join(distDir, 'Data/Platform/Distribution/RuntimeDependencies'),
        );
        cp(
          path.join(sourceDir, `src/platform_se/pex/TESModPlatform.pex`),
          path.join(distDir, 'Data/Scripts'),
        );
        cp(
          `${getBinaryDir()}/skymp5-server/cpp/${buildCfg}/MpClientPlugin.dll`,
          path.join(distDir, 'Data/SKSE/Plugins'),
        );
        fs.copySync(
          path.join(sourceDir, 'tools/plugin-example'),
          path.join(distDir, 'Data/Platform/plugin-example'),
        );
        fs.removeSync(
          path.join(distDir, 'Data/Platform/plugin-example/node_modules'),
        );
        fs.removeSync(path.join(distDir, 'Data/Platform/plugin-example/dist'));

        if (!process.env.DEV_SERVICE_NO_GAME) {
          if (config.SkyrimSEFolder !== 'OFF' && config.SkyrimSEFolder !== '') {
            fs.copySync(distDir, config.SkyrimSEFolder);
          }
        }

        // No need to release pdb to the public
        unlinkIfExists(path.join(distDir, 'SkyrimPlatform.pdb'));
        unlinkIfExists(path.join(distDir, 'SkyrimPlatformCEF.pdb'));
        unlinkIfExists(path.join(distDir, 'SkyrimPlatformImpl.pdb'));
      }

      // On Linux, we would not have this directory created yet
      createDirectory(path.join(distDir, 'Data/Platform/Modules'));

      // The native TS converter was removed. Old build directories can still
      // contain its output, so always package the current generated source.
      const spDotTsPath = path.join(
        sourceDir,
        'src/platform_se/codegen/convert-files/skyrimPlatform.ts',
      );
      if (!fs.existsSync(spDotTsPath)) {
        throw new Error(`Cannot find skyrimPlatform.ts in ${spDotTsPath}`);
      }
      cp(spDotTsPath, path.join(distDir, 'Data/Platform/Modules'));

      if (!process.env.DEV_SERVICE_NO_GAME) {
        if (config.SkyrimSEFolder === 'OFF' || config.SkyrimSEFolder === '') {
          console.log(
            `It seems that you didn't specify SKYRIM_DIR CMake option. The game will not be restarted.`,
          );
        } else {
          console.log(`Starting ${config.SkyrimSEFolder}`);
          game.launch(config.SkyrimSEFolder).catch((e) => console.error(e));
        }
      }
    }
  }
};

if (process.env.DEV_SERVICE_ONLY_ONCE) {
  const defaultConfig =
    normalizeBuildConfig(process.env.DEFAULT_CONFIG) ||
    normalizeBuildConfig(process.env.BUILD_TYPE) ||
    'Release';
  watchCallback(undefined, `touch_${defaultConfig}`);
} else {
  console.log(`Watching for changes in ${bin}`);
  fs.watch(bin, watchCallback);
}
