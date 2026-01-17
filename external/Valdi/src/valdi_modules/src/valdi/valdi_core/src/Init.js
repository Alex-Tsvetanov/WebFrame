(function () {
  function loadRootModule(jsEvaluator, path) {
    var exports = {};
    var module = {};
    module.exports = exports;

    const requireFunc = path => {
      if (path === 'tslib') {
        return loadRootModule(jsEvaluator, `valdi_core/src/tslib`);
      } else {
        throw new Error(`Invalid root module ${path}`);
      }
    };

    jsEvaluator(path, requireFunc, module, exports);

    return module.exports;
  }

  const jsEvaluator = runtime.loadJsModule;

  const moduleLoader = loadRootModule(jsEvaluator, 'valdi_core/src/ModuleLoader');
  const commonjsModuleLoaderType = runtime.moduleLoaderType === 'commonjs';
  const instance = moduleLoader.create(jsEvaluator, runtime.getSourceMap, runtime.loadModule, commonjsModuleLoaderType);

  const isBrowser = typeof window !== 'undefined' && typeof window.document !== 'undefined';
  if (!isBrowser) {
    // Valdi is probably alone in this js runtime, overwrite whatever globals you want
    global.require = instance.load.bind(instance);
  } else {
    // Valdi is not alone on the web, don't overwrite require for everyone.
    global.valdiRequire = instance.load.bind(instance);
  }
  global.moduleLoader = instance;
  global.Long = instance.load('valdi_core/src/Long', true);

  instance.load('valdi_core/src/PostInit', true).postInit();
})();
