import { RequireFunc } from 'valdi_core/src/IModuleLoader';

declare const require: RequireFunc;
const importedModule = require('./ModuleLoaderTestChild');
const importedScopedModule = require('./@fakescope/ModuleLoaderTestChildScoped');

export function getImportedType(): string {
  return typeof importedModule;
}

export function getScopedImportedType(): string {
  return typeof importedScopedModule;
}
