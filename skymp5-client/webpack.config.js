const path = require('path');
var mode = process.env.NODE_ENV || 'development';

module.exports = {
  mode: "development",
  //mode: "production",
  entry: [
    "./src/index.ts"
  ],
  devtool: (mode === 'development') ? 'inline-source-map' : false,
  output: {
    path: path.resolve(__dirname, '../build/dist/client/Data/Platform/Plugins'),
    filename: "skymp5-client.js",
    libraryTarget: "commonjs",
  },
  // performance: {
  //   maxAssetSize: 1024000,
  // },
  resolve: {
    // modules: [
    //   path.resolve(__dirname, '../build/dist/client/Data/Platform/Modules'),
    //   "node_modules"
    // ],
    extensions: [".ts", ".tsx", ".js"],
    alias: {
      "skyrimPlatform": path.resolve(__dirname, '../build/dist/client/Data/Platform/Modules/skyrimPlatform.ts'),
    }
  },
  optimization: {
    minimize: false
  },
  module: {
    rules: [
      {
        test: /.*skyrimPlatform.ts$/,
        use: path.resolve('src/loaders/skyrimPlatformLoader.js'),
      },
      {
        test: /\.tsx?$/,
        use: "ts-loader"
      }
    ]
  }
};