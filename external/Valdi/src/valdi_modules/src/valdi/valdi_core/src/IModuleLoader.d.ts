import { GetOrCreateSourceMapFunc } from 'source_map/src/StackSymbolicator';

export type RequireFunc = (path: string, disableProxy?: boolean, disableSyncDependencies?: boolean) => any;
export type OnHotReloadCallback = () => void;
export type OnHotReloadFunc = (
  observingModule: { path: string },
  path: string,
  callback: OnHotReloadCallback,
) => () => void;
export type OnModuleRegisteredCallback = () => void;

export interface IModuleLoader {
  load: RequireFunc;
  unload(paths: string[], isHotReloading: boolean, disableHotReloadDenyList: boolean): string[];
  unloadAllUnused(modulesToKeep: string[]): string[];
  preload(path: string, maxDepth: number): void;
  isLoaded(path: string): boolean;
  hasModuleFactory(path: string): boolean;
  unregisterModule(path: string): void;
  registerModule(path: string, factory: () => any): void;
  onModuleRegistered(path: string, callback: OnModuleRegisteredCallback): void;
  resolveRequire(path: string): RequireFunc;
  getOrCreateSourceMap: GetOrCreateSourceMapFunc;
  onHotReload: OnHotReloadFunc;
}
