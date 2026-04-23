import fs from 'fs';
import path from 'path';

import { getModsSnapshot } from './mods-service.js';

/**
 * Environment and workspace checks for SkyMP development.
 */
export function runDoctor(repoRoot, ctx) {
  /** @type {{ level: string; code: string; message: string }[]} */
  const issues = [];

  const vcpkgToolchain = path.join(repoRoot, 'vcpkg', 'scripts', 'buildsystems', 'vcpkg.cmake');
  if (!fs.existsSync(vcpkgToolchain)) {
    issues.push({
      level: 'error',
      code: 'vcpkg_missing',
      message:
        'vcpkg toolchain not found (submodule?). Run: git submodule init && git submodule update',
    });
  }

  const cmakeLists = path.join(repoRoot, 'CMakeLists.txt');
  if (!fs.existsSync(cmakeLists)) {
    issues.push({
      level: 'error',
      code: 'not_skymp_repo',
      message: `CMakeLists.txt missing under ${repoRoot}`,
    });
  }

  if (!fs.existsSync(ctx.buildDir)) {
    issues.push({
      level: 'warn',
      code: 'build_dir_missing',
      message: `CMake build directory not found: ${ctx.buildDir} (configure CMake first).`,
    });
  }

  if (!ctx.skyrimRoot) {
    issues.push({
      level: 'warn',
      code: 'skyrim_dir_missing',
      message:
        'SKYRIM_DIR / skyrimRoot not set - game-side sync will be skipped. Use .skymp-dev.yaml or SKYRIM_DIR.',
    });
  } else {
    if (!fs.existsSync(ctx.skyrimExePath)) {
      issues.push({
        level: 'error',
        code: 'bad_skyrim_root',
        message: `SkyrimSE.exe not found under skyrimRoot: ${ctx.skyrimRoot}`,
      });
    } else {
      if (!fs.existsSync(ctx.skseLoaderPath)) {
        issues.push({
          level: 'warn',
          code: 'skse_loader_missing',
          message: `skse64_loader.exe not found at ${ctx.skseLoaderPath}`,
        });
      }

      const skyrimPlatformDll = path.join(ctx.sksePluginsDir, 'SkyrimPlatform.dll');
      if (!fs.existsSync(skyrimPlatformDll)) {
        issues.push({
          level: 'warn',
          code: 'skyrim_platform_missing',
          message:
            `Skyrim Platform runtime is missing from the game install: ${skyrimPlatformDll}`,
        });
      }

      const skympClientPlugin = path.join(ctx.skyrimPlatformPluginDir, 'skymp5-client.js');
      if (!fs.existsSync(skympClientPlugin)) {
        issues.push({
          level: 'warn',
          code: 'skymp_client_plugin_missing',
          message: `SkyMP client plugin is missing from the game install: ${skympClientPlugin}`,
        });
      }
    }
  }

  if (ctx.skyrimRoot && ctx.skyrimPlatformDir) {
    const pluginsDevClient = path.join(ctx.skyrimPlatformPluginsDevDir, 'skymp5-client.js');
    if (fs.existsSync(pluginsDevClient)) {
      issues.push({
        level: 'warn',
        code: 'pluginsdev_override',
        message: `Stale PluginsDev client may override the real plugin: ${pluginsDevClient}`,
      });
    }
  }

  const nirnSibling = path.join(path.dirname(repoRoot), 'NirnLabUIPlatform');
  const nirnDll = path.join(ctx.nirnLabOutputDir, 'Data', 'SKSE', 'Plugins', 'NirnLabUIPlugin.dll');
  if (fs.existsSync(nirnSibling) && !fs.existsSync(nirnDll)) {
    issues.push({
      level: 'warn',
      code: 'nirnlab_build_missing',
      message: `NirnLab checkout present but plugin missing: ${nirnDll}`,
    });
  }

  if (ctx.skyrimRoot && fs.existsSync(ctx.skyrimExePath)) {
    const modsSnapshot = getModsSnapshot({ ctx });

    if (!modsSnapshot.pluginsTxt.path) {
      issues.push({
        level: 'warn',
        code: 'plugins_txt_path_missing',
        message:
          'Plugins.txt path could not be resolved. Configure pluginsTxtPath if you want the Mods tab to control plugin-backed mods.',
      });
    } else if (modsSnapshot.pluginsTxt.readable === false) {
      issues.push({
        level: 'warn',
        code: 'plugins_txt_unreadable',
        message:
          `Plugins.txt could not be read at ${modsSnapshot.pluginsTxt.path}: ${modsSnapshot.pluginsTxt.readError}`,
      });
    } else if (
      modsSnapshot.summary.enabled + modsSnapshot.summary.disabled > 0 &&
      modsSnapshot.pluginsTxt.exists === false
    ) {
      issues.push({
        level: 'warn',
        code: 'plugins_txt_missing',
        message:
          `Plugins.txt does not exist yet at ${modsSnapshot.pluginsTxt.path}. Plugin-backed mods will be treated as disabled until the file is created.`,
      });
    }

    if (modsSnapshot.pluginsTxt.duplicatePluginNames.length > 0) {
      issues.push({
        level: 'warn',
        code: 'plugins_txt_duplicates',
        message:
          `Plugins.txt contains duplicate plugin entries: ${modsSnapshot.pluginsTxt.duplicatePluginNames.slice(0, 5).join(', ')}`,
      });
    }

    if (modsSnapshot.pluginsTxt.conflictingPluginNames.length > 0) {
      issues.push({
        level: 'warn',
        code: 'plugins_txt_conflicts',
        message:
          `Plugins.txt contains conflicting enabled/disabled duplicates: ${modsSnapshot.pluginsTxt.conflictingPluginNames.slice(0, 5).join(', ')}`,
      });
    }

    const missingPluginMods = modsSnapshot.mods.filter((mod) => mod.kind === 'missing_plugin');
    if (missingPluginMods.length > 0) {
      issues.push({
        level: 'warn',
        code: 'plugins_missing_on_disk',
        message:
          `Plugins.txt references plugin files that are missing from Data: ${missingPluginMods.slice(0, 5).map((mod) => mod.pluginName).join(', ')}`,
      });
    }
  }

  const ok = issues.filter((i) => i.level === 'error').length === 0;

  return { ok, issues, ctx };
}
