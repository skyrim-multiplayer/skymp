const path = require("path");
const fs = require("fs");
const HtmlWebpackPlugin = require("html-webpack-plugin");

if (!fs.existsSync("config.js")) {
  fs.writeFileSync("config.js", fs.readFileSync("config-default.js"));
}

const config = require("./config");

const distPath = path.isAbsolute(config.ui.outputPath)
  ? config.ui.outputPath
  : path.resolve(__dirname, config.ui.outputPath);

module.exports = {
  entry: path.resolve(__dirname, "src/index.js"),
  output: {
    path: distPath,
    filename: "build.js",
  },
  mode: "development",
  devServer: {
    contentBase: distPath,
    port: 1234,
  },
  plugins: [
    new HtmlWebpackPlugin({
      template: path.resolve(__dirname, "public/index.html"),
    }),
  ],
  module: {
    rules: [
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
        test: /\.(png|svg|jpg|gif)$/,
        use: "file-loader",
      },
      {
        test: /\.(woff|woff2|eot|ttf|otf)$/,
        use: "file-loader",
      },
    ],
  },
};
