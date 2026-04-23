import fs from 'fs';
import path from 'path';

/**
 * Choose NirnLab output dir: sibling checkout first, then in-repo (same rules as sync-dev-runtime.ps1).
 */
export function resolveNirnLabOutputDir(repoRoot, config) {
  if (config?.nirnLabOutputDir) {
    return path.normalize(config.nirnLabOutputDir);
  }
  const repoParent = path.dirname(repoRoot);
  const nirnLabSiblingRepo = path.join(repoParent, 'NirnLabUIPlatform');
  const nirnLabInRepo = path.join(repoRoot, 'NirnLabUIPlatform');
  if (fs.existsSync(nirnLabSiblingRepo)) {
    return path.join(nirnLabSiblingRepo, 'build', 'dist', 'Release');
  }
  return path.join(nirnLabInRepo, 'build', 'dist', 'Release');
}

export function resolvePluginsTxtPath(config) {
  if (config?.pluginsTxtPath) {
    return path.normalize(config.pluginsTxtPath);
  }

  if (process.platform !== 'win32') {
    return '';
  }

  const localAppData =
    process.env.LOCALAPPDATA || path.join(process.env.USERPROFILE || '', 'AppData', 'Local');
  if (!localAppData) {
    return '';
  }

  return path.join(localAppData, 'Skyrim Special Edition', 'Plugins.txt');
}

/**
 * Build variable context for manifest expansion.
 * @param {object} opts
 * @param {string} opts.repoRoot
 * @param {object} [opts.config] - from .skymp-dev.yaml
 */
export function buildContext({ repoRoot, config = {} }) {
  const buildDir =
    config.buildDir != null ? path.normalize(config.buildDir) : path.join(repoRoot, 'build');
  const skyrimRoot =
    config.skyrimRoot ||
    process.env.SKYRIM_DIR?.replace(/\\/g, '/') ||
    process.env.SKYRIM_DIR ||
    '';

  const ctx = {
    repoRoot: path.normalize(repoRoot),
    repoParent: path.normalize(path.dirname(repoRoot)),
    buildDir,
    skyrimRoot: skyrimRoot ? path.normalize(skyrimRoot) : '',
    pluginsTxtPath: resolvePluginsTxtPath(config),
    skyrimExePath: '',
    skseLoaderPath: '',
    skyrimPlatformDir: '',
    skyrimPlatformPluginDir: '',
    skyrimPlatformPluginsDevDir: '',
    skyrimPlatformRuntimeDir: '',
    skyrimDataDir: '',
    sksePluginsDir: '',
    nirnLabOutputDir: resolveNirnLabOutputDir(repoRoot, config),
  };

  if (ctx.skyrimRoot) {
    ctx.skyrimExePath = path.join(ctx.skyrimRoot, 'SkyrimSE.exe');
    ctx.skseLoaderPath = config?.skseLoaderPath
      ? path.normalize(config.skseLoaderPath)
      : path.join(ctx.skyrimRoot, 'skse64_loader.exe');
    ctx.skyrimPlatformDir = path.join(ctx.skyrimRoot, 'Data', 'Platform');
    ctx.skyrimPlatformPluginDir = path.join(ctx.skyrimPlatformDir, 'Plugins');
    ctx.skyrimPlatformPluginsDevDir = path.join(ctx.skyrimPlatformDir, 'PluginsDev');
    ctx.skyrimPlatformRuntimeDir = path.join(
      ctx.skyrimPlatformDir,
      'Distribution',
      'RuntimeDependencies',
    );
    ctx.skyrimDataDir = path.join(ctx.skyrimRoot, 'Data');
    ctx.sksePluginsDir = path.join(ctx.skyrimDataDir, 'SKSE', 'Plugins');
  }

  return ctx;
}
