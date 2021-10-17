//webpack.config.js
const path = require('path');

module.exports = {
  mode: "development",
  //mode: "production",
  // entry: {
  //     main: "./src/index.ts",
  // },
  entry: [
    "./src/index.ts"
  ],
  //devtool: "inline-source-map",
  output: {
    libraryTarget: "commonjs",
    //path: path.resolve(__dirname, '../build/dist/server/dist_front'),
    path: path.resolve(__dirname, '../build/dist/client/Data/Platform/Plugins'),
    filename: "skymp5-client.js",
  },
  resolve: {
    // modules: [
    //   path.resolve(__dirname, '../build/dist/client/Data/Platform/Modules'),
    //   "node_modules"
    // ],
    extensions: [".ts", ".tsx", ".js"],
    alias: {
      "skyrimPlatform": path.resolve(__dirname, '../build/dist/client/Data/Platform/Modules/skyrimPlatform.ts'),
      // "skyrimPlatform": "./spAPI.ts",
    }
  },
  optimization: {
    minimize: true
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