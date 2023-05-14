const path = require('path');
// const outDirPath = path.resolve(__dirname, "../build/dist/client/Data/Platform/Plugins");
const outDirPath = "F:/Program Files (x86)/Steam/steamapps/common/Skyrim Special Edition/Data/Platform/Plugins";
const outFileName = "skymp5-client.js";
const spFilePath = path.resolve(__dirname, '../build/dist/client/Data/Platform/Modules/skyrimPlatform.ts');

module.exports = {
  mode: "development",
  entry: [
    "./src/index.ts"
  ],
  devtool: false,
  output: {
    path: outDirPath,
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
      use: path.resolve('loaders/skyrimPlatformLoader.js'),
    }, {
      test: /\.tsx?$/,
      use: "ts-loader"
    }]
  }
};
