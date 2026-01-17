import { StringMap } from 'coreutils/src/StringMap';
import { ComponentConstructor, IComponent } from './IComponent';
import { RequireFunc } from './IModuleLoader';

export interface ComponentPath {
  filePath: string;
  symbolName: string;
}

const cache: StringMap<ComponentPath> = {};

export function parseComponentPath(componentPath: string): ComponentPath {
  const cached = cache[componentPath];
  if (cached) {
    return cached;
  }

  const symbolSeparator = componentPath.indexOf('@');

  if (symbolSeparator >= 0) {
    let symbolName = componentPath.substring(0, symbolSeparator);
    if (symbolName.indexOf('/') >= 0) {
      // TODO(simon): Compat with legacy navigation on Android which automatically
      // prepend the module name to the path
      const components = symbolName.split('/');
      symbolName = components[components.length - 1];
    }

    const filePath = componentPath.substring(symbolSeparator + 1);
    const value = { filePath, symbolName };
    cache[componentPath] = value;
    return value;
  }

  const legacyModuleSeparator = componentPath.indexOf(':');
  if (legacyModuleSeparator >= 0) {
    const moduleName = componentPath.substring(0, legacyModuleSeparator);
    const filePath = componentPath.substring(legacyModuleSeparator + 1);
    return parseComponentPath(moduleName + '/' + filePath);
  }

  // Backward compatibility with .valdi syntax
  if (componentPath.endsWith('.valdi')) {
    const prefix = componentPath.substr(0, componentPath.length - '.valdi'.length);
    const value = { filePath: `${prefix}.vue.generated`, symbolName: 'ComponentClass' };
    cache[componentPath] = value;
    return value;
  }

  throw Error(`Invalid componentPath ${componentPath}`);
}

export function resolveComponentConstructor<T extends IComponent>(
  require: RequireFunc,
  componentPath: string | ComponentPath,
): ComponentConstructor<T> {
  let parsedPath: ComponentPath;
  if (typeof componentPath === 'string') {
    parsedPath = parseComponentPath(componentPath);
  } else {
    parsedPath = componentPath;
  }

  const module = require(parsedPath.filePath, true);
  const constructor = module[parsedPath.symbolName];
  if (!constructor) {
    throw Error(`Could not resolve component ${parsedPath.symbolName} constructor in ${parsedPath.filePath}`);
  }

  return constructor;
}
