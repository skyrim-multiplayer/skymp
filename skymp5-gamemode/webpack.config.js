const path = require('path')

let signFile = () => {}
try {
  signFile = require('../sign-gamemode').signFile
} catch (e) {
  // sign-gamemode not available locally, skipping signing
}

const outputDir = process.env.GAMEMODE_OUTPUT_DIR
  ? path.resolve(process.env.GAMEMODE_OUTPUT_DIR)
  : path.resolve(__dirname, '..')

const outputFile = path.join(outputDir, 'gamemode.js')

class SignGamemodePlugin {
  apply(compiler) {
    compiler.hooks.done.tap('SignGamemodePlugin', (stats) => {
      if (stats.hasErrors()) return
      signFile(outputFile)
    })
  }
}

/** @type {import('webpack').Configuration} */
module.exports = {
  entry: './src/index.ts',
  target: 'node',
  mode: 'production',
  module: {
    rules: [
      {
        test: /\.tsx?$/,
        use: {
          loader: 'ts-loader',
          options: {
            configFile: path.resolve(__dirname, 'tsconfig.json'),
          },
        },
        exclude: /node_modules/,
      },
    ],
  },
  resolve: {
    extensions: ['.ts', '.js'],
  },
  output: {
    filename: 'gamemode.js',
    path: outputDir,
    library: {
      type: 'commonjs2',
    },
  },
  optimization: {
    minimize: false,
  },
  plugins: [new SignGamemodePlugin()],
}
