import { IModuleLoader } from './IModuleLoader';

declare const global: any;

export function getModuleLoader(): IModuleLoader {
  return global.moduleLoader as IModuleLoader;
}
