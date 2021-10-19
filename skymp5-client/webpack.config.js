const path = require('path');

const outDir = path.resolve(__dirname, "../build/dist/client/Data/Platform/Plugins");
const outFileName = "skymp5-client.js";
const spFilePath = path.resolve(__dirname, '../build/dist/client/Data/Platform/Modules/skyrimPlatform.ts');

module.exports = {
  mode: "development",
  entry: [
    "./src/index.ts"
  ],
  devtool: false,
  output: {
    path: outDir,
    filename: outFileName,
    libraryTarget: "commonjs",
  },
  resolve: {
    extensions: [".ts", ".tsx", ".js"],
    alias: {
      "skyrimPlatform": spFilePath,
    }
  },
  optimization: {
    minimize: false
  },
  module: {
    rules: [{
      test: /.*skyrimPlatform.ts$/,
      use: path.resolve('src/loaders/skyrimPlatformLoader.js'),
    }, {
      test: /\.tsx?$/,
      use: "ts-loader"
    }]
  }
};