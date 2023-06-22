let fs = require("fs-extra");
let path = require("path");
let childProcess = require("child_process");
let game = require("./game");

function writeFileSyncRecursive(filename, content, charset) {
  filename
    .split(path.sep)
    .slice(0, -1)
    .reduce((last, folder) => {
      let folderPath = last ? last + path.sep + folder : folder;
      if (!fs.existsSync(folderPath)) fs.mkdirSync(folderPath);
      return folderPath;
    });

  fs.writeFileSync(filename, content, charset);
}

if (!fs.existsSync("config.js")) {
  fs.writeFileSync("config.js", fs.readFileSync("config-default.js"));
}
let config = require("./config");

console.log("Dev service started");

const getBinaryDir = () => path.resolve(__dirname, "../../../build");
const getSourceDir = () => path.resolve(__dirname, "../..");

console.log(`Binary dir is '${getBinaryDir()}'`);
console.log(`Source dir is '${getSourceDir()}'`);

let bin = path.join(getBinaryDir(), "skyrim-platform/_platform_se");
let sourceDir = path.join(getSourceDir());
let distDir = path.join(getBinaryDir(), "dist/client");

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
          process.platform !== "win32" ||
          e.toString().indexOf("EPERM") === -1
        ) {
          throw e;
        }
      }
  }
};

const watchCallback = (_eventType, fileName) => {
  {
    if (fileName === "touch_Release" || fileName === "touch_Debug") {
      let buildCfg = fileName === "touch_Release" ? "Release" : "Debug";

      console.log("Skyrim Platform " + buildCfg + " x64 updated.");

      if (!process.env.DEV_SERVICE_NO_GAME) {
        console.log("Stopping Skyrim SE...");
        game.kill();
      }

      // All except "Plugins" since we need RestartGame target to keep Plugins directory alive in watch mode
      const directoriesToClear = [
        path.join(distDir, "Data/Platform/Distribution"),
        path.join(distDir, "Data/Platform/Modules"),
        path.join(distDir, "Data/Platform/plugin-example"),
      ];
      directoriesToClear.forEach((directory) => {
        if (fs.existsSync(directory)) {
          fs.removeSync(directory);
        }
      });
      createDirectory(distDir);
      let getFileName = (p) => p.replace(/^.*[\\\/]/, "");
      let cp = (from, targetDir) =>
        writeFileSyncRecursive(
          path.join(targetDir, getFileName(from)),
          fs.readFileSync(from)
        );
      let binPath = (file) => path.join(bin, `bin/${buildCfg}/${file}`);

      if (process.platform === "win32") {
        let cefDir = fs
          .readFileSync(path.join(bin, `cef_dir.txt`))
          .toString("utf-8");
        [
          "chrome_elf.dll",
          "d3dcompiler_47.dll",
          "libcef.dll",
          "libEGL.dll",
          "libGLESv2.dll",
          "snapshot_blob.bin",
          "v8_context_snapshot.bin",
        ].forEach((item, i) => {
          cp(
            path.join(cefDir, "Release", item),
            path.join(distDir, "Data/Platform/Distribution/RuntimeDependencies")
          );
        });
        ["libEGL.dll", "libGLESv2.dll"].forEach((item, i) => {
          cp(
            path.join(cefDir, "Release", item),
            path.join(
              distDir,
              "Data/Platform/Distribution/RuntimeDependencies"
            )
          );
        });
        ["icudtl.dat"].forEach((item, i) => {
          cp(
            path.join(cefDir, "Resources", item),
            path.join(distDir, "Data/Platform/Distribution/RuntimeDependencies")
          );
        });
        [
          "chrome_100_percent.pak",
          "chrome_200_percent.pak",
          "resources.pak",
        ].forEach((item, i) => {
          cp(
            path.join(cefDir, "Resources", item),
            path.join(distDir, "Data/Platform/Distribution/CEF")
          );
        });
        fs.copySync(
          path.join(cefDir, "Resources/locales"),
          path.join(distDir, "Data/Platform/Distribution/CEF/locales")
        );

        cp(binPath("SkyrimPlatform.pdb"), distDir);
        cp(binPath("SkyrimPlatformImpl.pdb"), distDir);
        cp(
          binPath("ChakraCore.dll"),
          path.join(distDir, "Data/Platform/Distribution/RuntimeDependencies")
        );
        cp(
          binPath("SkyrimPlatformCEF.exe.hidden"),
          path.join(distDir, "Data/Platform/Distribution/RuntimeDependencies")
        );
        cp(binPath("SkyrimPlatformCEF.pdb"), distDir);
        cp(
          binPath("SkyrimPlatform.dll"),
          path.join(distDir, "Data/SKSE/Plugins")
        );
        cp(
          binPath("SkyrimPlatformImpl.dll"),
          path.join(distDir, "Data/Platform/Distribution/RuntimeDependencies")
        );
        cp(
          path.join(sourceDir, `src/platform_se/pex/TESModPlatform.pex`),
          path.join(distDir, "Data/Scripts")
        );
        cp(
          `${getBinaryDir()}/skymp5-server/cpp/${buildCfg}/MpClientPlugin.dll`,
          path.join(distDir, "Data/SKSE/Plugins")
        );
        cp(
          path.join(
            sourceDir,
            "tools/system_polyfill/dist/___systemPolyfill.js"
          ),
          path.join(distDir, "Data/Platform/Distribution")
        );
        fs.copySync(
          path.join(sourceDir, "tools/plugin-example"),
          path.join(distDir, "Data/Platform/plugin-example")
        );
        fs.removeSync(
          path.join(distDir, "Data/Platform/plugin-example/node_modules")
        );
        fs.removeSync(path.join(distDir, "Data/Platform/plugin-example/dist"));

        if (!process.env.DEV_SERVICE_NO_GAME) {
          if (config.SkyrimSEFolder !== "OFF" && config.SkyrimSEFolder !== "") {
            fs.copySync(distDir, config.SkyrimSEFolder);
          }
        }

        // No need to release pdb to the public
        fs.unlinkSync(path.join(distDir, "SkyrimPlatform.pdb"));
        fs.unlinkSync(path.join(distDir, "SkyrimPlatformCEF.pdb"));
        fs.unlinkSync(path.join(distDir, "SkyrimPlatformImpl.pdb"));
      }

      // On Linux, we would not have this directory created yet
      createDirectory(path.join(distDir, "Data/Platform/Modules"));
      cp(
        path.join(bin, `_codegen/skyrimPlatform.ts`),
        path.join(distDir, "Data/Platform/Modules")
      );
      cp(
        path.join(bin, `_codegen/skyrimPlatform.ts`),
        path.join(sourceDir, "src/platform_se/codegen/convert-files")
      );

      if (!process.env.DEV_SERVICE_NO_GAME) {
        if (config.SkyrimSEFolder === "OFF" || config.SkyrimSEFolder === "") {
          console.log(
            `It seems that you didn't specify SKYRIM_DIR CMake option. The game will not be restarted.`
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
    process.env.DEFAULT_CONFIG === "Debug" ? "Debug" : "Release";
  watchCallback(undefined, `touch_${defaultConfig}`);
} else {
  console.log(`Watching for changes in ${bin}`);
  fs.watch(bin, watchCallback);
}
