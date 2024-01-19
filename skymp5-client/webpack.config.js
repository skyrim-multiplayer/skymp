const path = require('path');

const outDirPath = path.resolve(__dirname, "../build/dist/client/Data/Platform/Plugins");
const outFileName = "skymp5-client.js";
const spFilePath = path.resolve(__dirname, './node_modules/@skyrim-platform/skyrim-platform/index.d.ts');

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
    rules: [
      {
        test: /\.tsx?$/,
        use: "ts-loader",
        exclude: /node_modules/,
      },
      {
		test: /.*skyrimPlatform.ts$/,
		use: path.resolve('loaders/skyrimPlatformLoader.js'),
	  },	
      {
        test: /\.d\.ts$/,
        use: "ignore-loader",
      },
    ],
  },
};
