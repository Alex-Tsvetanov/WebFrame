const path = require('path');

module.exports = {
  mode: 'development',
  target: 'node',
  entry: './src/index.ts',
  devtool: 'source-map',
  plugins: [],
  module: {
    rules: [
      {
        test: /\.([cm]?ts|tsx)$/,
        include: [
          path.resolve(__dirname, './src'),
          path.resolve(__dirname, './remotedebug-ios-webkit-adapter'),
          path.resolve(__dirname, './supported-locales')
        ],
        exclude: /node_modules/,
        loader: 'ts-loader',
      },
      {
        test: /\.node$/,
        loader: 'node-loader',
        options: {
          name: '[name].[ext]',
        },
      },
    ],
  },
  resolve: {
    // Add `.ts` and `.tsx` as a resolvable extension.
    extensions: ['.ts', '.tsx', '.js'],
    // Add support for TypeScripts fully qualified ESM imports.
    extensionAlias: {
      '.js': ['.js', '.ts'],
      '.cjs': ['.cjs', '.cts'],
      '.mjs': ['.mjs', '.mts'],
    },
  },
  output: {
    filename: 'bundle.js',
    path: path.resolve(__dirname, 'dist_dev'),
  },
};
