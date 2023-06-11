const path = require("path");
const fs = require("fs");
const HtmlWebpackPlugin = require("html-webpack-plugin");
const config = require("./config");

const distPath = path.isAbsolute(config.outputPath)
  ? config.outputPath
  : path.resolve(__dirname, config.outputPath);

module.exports = {
  entry: path.resolve(__dirname, "src/index.js"),
  output: {
    path: distPath,
    filename: "build.js",
  },
  mode: "development",
  devServer: {
    port: 1234,
    hot: true,
  },
  plugins: [
    new HtmlWebpackPlugin({
      template: path.resolve(__dirname, "public/index.html"),
    }),
  ],
  module: {
    rules: [
      {
        test: /\.(js|jsx)$/,
        exclude: /node_modules/,
        use: ['babel-loader'],
      },
      { test: /\.tsx?$/, loader: 'ts-loader' },
      {
        test: /\.m?js$/,
        exclude: /(node_modules|bower_components|bridge)/,
        use: {
          loader: "babel-loader",
        },
      },
      {
        test: /\.s[ac]ss/i,
        use: ["style-loader", "css-loader", "sass-loader"],
      },
      {
        test: /\.css/i,
        use: ["style-loader", "css-loader"],
      },
      {
        test: /\.(png|svg|jpg|gif|mp3|wav)$/,
        use: "file-loader",
      },
    ],
  },
  resolve: {
    extensions: ['*', '.js', '.jsx', '.ts', '.tsx'],
  },
};
