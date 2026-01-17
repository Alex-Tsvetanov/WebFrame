# Better SQLite-3 Bindings

We ship the companion code as a single dist folder with a bundle.js file. This bundle needs to support all platforms we support, which include Linux x86_64, MacOS arm64 and MacOS x86_64.
When running `npm install`, it will install and build the better-sqlite3 bindings for the platform it is running on only. In order to support all platforms, we include in this folder the .node files for every platform, which were copied then from `node_modules/better-sqlite3/build/Release/better_sqlite3.node`.
To update the bindings, you need to run `npm install` on all platforms we support and copy the files from `node_modules/better-sqlite3/build/Release/better_sqlite3.node`.
