const path = require('path');
const HtmlWebpackPlugin = require("html-webpack-plugin");
const webpack = require('webpack');

module.exports = {
  entry: './src/index.js',
  output: {
    filename: 'bundle.js',
    path: path.resolve(__dirname, 'dist'),
    clean: true,
  },
  mode: 'development',
  devtool: 'source-map',
  ignoreWarnings: [{ message: /Cannot statically analyse 'require/ }],
  plugins: [
    new HtmlWebpackPlugin({ template: "src/index.html" }),
    new webpack.IgnorePlugin({
      resourceRegExp: /foundation[\\\/]test[\\\/]util[\\\/]lib[\\\/]faker\.js$/
    }),
  ],
  devServer: { port: 3030, hot: true },
  resolve: {
    extensions: ['.js', '.jsx', '.ts', '.tsx'],
    modules: [path.resolve(__dirname, 'node_modules')],
  },
  module: {
    rules: [
      { test: /\.(sa|sc|c)ss$/, use: ["style-loader", "css-loader", "sass-loader"] },
      {
        test: /\.jsx?$/,
        exclude: /node_modules/,
        use: { 
          loader: 'babel-loader',
          options: {
            presets: [
              ['@babel/preset-env', { targets: { esmodules: true } }],
              ['@babel/preset-react', { runtime: 'automatic' }],
            ],
          },
        },
      },
      // BEGIN REQUIRED FOR VALDI WEB
      // (url-loader is deprecated in webpack 5; asset modules work too, but keeping as-is if you need)
      { test: /\.(png|woff|woff2|eot|ttf|svg)$/, loader: "url-loader", options: { limit: false } },
      { test: /\.protodecl$/, type: 'asset/resource' },
      {
        test: /\.tsx?$/,
        loader: 'ts-loader',
        exclude: /node_modules/,
        options: { appendTsxSuffixTo: [/\.vue$/], transpileOnly: true },
      },
      // END REQUIRED FOR VALDI WEB
    ],
  },
};