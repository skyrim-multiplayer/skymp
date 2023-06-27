const fs = require('fs');
const path = require('path');
const packageInfo = require('./package.json')
const WebpackShellPluginNext = require('webpack-shell-plugin-next');
const { getFips } = require('crypto');

// Configure webpack output file folder and file name
const outputFolder = path.resolve(__dirname, './build')
const outputFilename = `${packageInfo.name}.js`

// Configure entry point of application.
// This should be a .ts file, e.g. not dist/index.js
const entryPoint = './src/index.ts'

// When webpack is build normally, it will simply output a file to the configured output folder and filename.
// If you run webpack with the DEPLOY_PLUGIN environment variable configured to true, then it will
// additionally 'deploy' the plugin to the configured Skyrim directory (via skyrim.json or SKYRIMPATH)
// If no existing folder path is found to the Skyrim folder, an error is displayed and the webpack is canceled. 
let plugins = []
if (process.env['DEPLOY_PLUGIN']?.includes('true')) {
    // Get the path to the Skyrim folder from skyrim.json and the SKYRIMPATH environment variable, if configured.
    let skyrimConfig
    if (fs.existsSync('./skyrim.json')) skyrimConfig = require('./skyrim.json')
    const skyrimPathFromConfig = skyrimConfig && skyrimConfig.skyrimFolder
    const skyrimPathFromEnvironment = process.env['SKYRIMPATH']

    // If the path set in skyrim.json exists on the file system, use it.
    // Otherwise, if the path set in the SKYRIMPATH environment variable exists on the file system, us it.
    let skyrimFolder
    if (fs.existsSync(skyrimPathFromConfig)) skyrimFolder = skyrimPathFromConfig
    if (! skyrimFolder)
        if (fs.existsSync(skyrimPathFromEnvironment)) skyrimFolder = skyrimPathFromEnvironment
    if (! skyrimFolder) {
        console.error('Please set valid path to your Skyrim folder in skyrim.json (or SKYRIMPATH environment variable)') 
        process.exit(1)
    }

    const outputFile = path.join(outputFolder, outputFilename)
    const skyrimPlatformPluginsDirectory = path.resolve(skyrimFolder, 'Data', 'Platform', 'PluginsDev')
    fs.mkdirSync(skyrimPlatformPluginsDirectory, { recursive: true })
    const pluginDestinationPath = path.join(skyrimPlatformPluginsDirectory, outputFilename)
    let copyCommand = `xcopy "${outputFile}" "${skyrimPlatformPluginsDirectory}" /I /Y && echo "Copied ${outputFilename} to ${pluginDestinationPath}"`
    if (! process.platform == 'win32')
        copyCommand = `cp "${outputFile}" "${skyrimPlatformPluginsDirectory}" && echo "Copied ${outputFilename} to ${pluginDestinationPath}"`
    plugins.push(new WebpackShellPluginNext({ onAfterDone: { scripts: [copyCommand] } }))

// When `npm run zip` is run, the project is compiled, webpack is run, and a zip file named `[project name]-[project version].zip`
// is created in the project folder. The .zip file can be distributed via mod managers, e.g. on sites on NexusMods
} else if (process.env['ZIP_PLUGIN']?.includes('true')) {
    const outputFile = path.join(outputFolder, outputFilename)
    const zipFile = `${packageInfo.name}-${packageInfo.version}.zip`
    const localPlatformFolder = path.join('.', 'Platform')
    const localPluginsFolder = path.join(localPlatformFolder, 'Plugins')
    const localPluginFiles = path.join(localPluginsFolder, '*.js')
    fs.rmdirSync(localPluginsFolder, { recursive: true, force: true })
    fs.mkdirSync(localPluginsFolder, { recursive: true })
    let zipCommand = `xcopy "${outputFile}" "${localPluginsFolder}" && npm run zip:cli -- "${zipFile}" "${localPluginFiles}" && RMDIR /S /Q "${localPluginsFolder}"`
    if (! process.platform == 'win32')
        zipCommand = `cp "${outputFile}" "${localPluginsFolder}" && npm run zip:cli -- "${zipFile}" "${localPluginFiles}" && rm -rf "${localPlatformFolder}"`
    plugins.push(new WebpackShellPluginNext({ onBuildEnd: { scripts: [zipCommand] } }))
}

module.exports = {
    plugins,
    mode: 'development',
    devtool: 'inline-source-map',
    entry: { main: entryPoint, },
    output: { path: outputFolder, filename: outputFilename },
    resolve: { extensions: ['.ts', '.tsx', '.js', '.jsx'] },
    externals: {
        // Do not bundle skyrim platform. Instead require('skyrimPlatform') at runtime
        '@skyrim-platform/skyrim-platform': ['skyrimPlatform'], // <--- support import 'skyrimPlatform'
        'skyrimPlatform': ['skyrimPlatform']
    },
    module: {
        rules: [{ test: /\.tsx?$/, loader: 'ts-loader', options: { configFile: 'tsconfig.json' }}]
    }
};
