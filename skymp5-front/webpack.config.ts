const path = require('path');
const fs = require('fs');
const webpack = require('webpack');
const HtmlWebpackPlugin = require('html-webpack-plugin');

const defaultConfig = {
  outputPath: 'dist',
  devServerPort: 1234,
  devServerHost: '0.0.0.0',
  devProxyTarget: 'http://127.0.0.1:7777',
  devHealthIntervalMs: 12000,
};

const tryLoadConfig = (fileName) => {
  const filePath = path.resolve(__dirname, fileName);
  if (!fs.existsSync(filePath)) {
    return null;
  }

  try {
    return require(filePath);
  } catch (e) {
    // Keep CI and local builds alive even if a TS config can't be loaded in CJS mode.
    console.warn(`[webpack-config] failed to load ${fileName}: ${e.message}`);
    return null;
  }
};

const loadFirstAvailableConfig = (fileNames) => {
  for (const fileName of fileNames) {
    const loaded = tryLoadConfig(fileName);
    if (loaded) {
      return loaded.default || loaded;
    }
  }
  return null;
};

// CI should work without local config files; developers can still override locally.
const config = {
  ...defaultConfig,
  ...(loadFirstAvailableConfig(['config.js', 'config.ts']) || {}),
  ...(loadFirstAvailableConfig(['config.local.js', 'config.local.ts']) || {}),
};

const distPath = path.isAbsolute(config.outputPath)
  ? config.outputPath
  : path.resolve(__dirname, config.outputPath);

const devServerPort = Number(
  process.env.SKYMP_FRONT_PORT || config.devServerPort || 1234,
);
const devServerHost =
  process.env.SKYMP_FRONT_HOST || config.devServerHost || '0.0.0.0';
const devProxyTarget =
  process.env.SKYMP_FRONT_API_TARGET ||
  config.devProxyTarget ||
  'http://127.0.0.1:7777';
const devHealthIntervalMs = Number(
  process.env.SKYMP_FRONT_HEALTH_MS || config.devHealthIntervalMs || 12000,
);

module.exports = {
  entry: path.resolve(__dirname, 'src/index.js'),
  output: {
    path: distPath,
    filename: 'build.js',
  },
  mode: 'development',
  devServer: {
    host: devServerHost,
    port: devServerPort,
    hot: true,
    allowedHosts: 'all',
    historyApiFallback: true,
    client: {
      overlay: true,
    },
    static: {
      directory: path.resolve(__dirname, 'public'),
    },
    proxy: {
      '/api': {
        target: devProxyTarget,
        changeOrigin: true,
        secure: false,
      },
    },
  },
  plugins: [
    new HtmlWebpackPlugin({
      template: path.resolve(__dirname, 'public/index.html'),
    }),
    new webpack.DefinePlugin({
      __SKYMP_DEV_SERVER_HOST__: JSON.stringify(devServerHost),
      __SKYMP_DEV_SERVER_PORT__: JSON.stringify(String(devServerPort)),
      __SKYMP_DEV_PROXY_TARGET__: JSON.stringify(devProxyTarget),
      __SKYMP_DEV_HEALTH_MS__: JSON.stringify(String(devHealthIntervalMs)),
    }),
  ],
  module: {
    rules: [
      {
        test: /\.(js|jsx)$/,
        exclude: /node_modules/,
        use: ['babel-loader'],
      },
      {
        test: /\.tsx?$/,
        loader: 'ts-loader',
        options: { transpileOnly: true },
      },
      {
        test: /\.m?js$/,
        exclude: /(node_modules|bower_components|bridge)/,
        use: {
          loader: 'babel-loader',
        },
      },
      {
        test: /\.s[ac]ss/i,
        use: [
          'style-loader',
          'css-loader',
          {
            loader: 'sass-loader',
            options: {
              implementation: require('sass'),
              api: 'modern',
            },
          },
        ],
      },
      {
        test: /\.css/i,
        use: ['style-loader', 'css-loader'],
      },
      {
        test: /\.(png|svg|jpg|gif|mp3|wav)$/,
        use: 'file-loader',
      },
    ],
  },
  resolve: {
    extensions: ['*', '.js', '.jsx', '.ts', '.tsx'],
  },
};
