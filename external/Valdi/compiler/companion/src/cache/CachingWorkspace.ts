import ts = require('typescript');
import { ILogger } from '../logger/ILogger';
import { EmitResult, GetDiagnosticsResult, IWorkspace, OpenFileImportPath, OpenFileResult } from '../IWorkspace';
import { FileManager, FilePath } from '../project/FileManager';
import {
  DumpEnumResponseBody,
  DumpFunctionResponseBody,
  DumpedSymbolsWithComments,
  DumpedInterface,
} from '../protocol';
import { rethrow } from '../utils/rethrow';
import { generateSHA256HashFromString, generateSHA256HashFromStrings } from '../utils/sha256';
import { ICompilationCache } from './ICompilationCache';
import * as _path from 'path';
import { Stopwatch } from '../utils/Stopwatch';
import { SerialTaskQueue } from '../utils/SerialTaskQueue';
import { CircularLoopTracker, CircularLoopTrackerPushResult } from './CircularLoopTracker';

interface ImportPaths {
  relatives: string[];
  absolutes: string[];
  isNonModuleWithAmbiantDeclarations: boolean;
}

interface CachedImportPaths {
  importPaths: OpenFileImportPath[];
  isNonModuleWithAmbiantDeclarations: boolean;
}

interface CachedFileInfo {
  shallowHash: string;
  filePath: FilePath;
  recursiveHash: string | undefined;
  importPaths: ImportPaths | undefined;
  dependentFiles: Set<string> | undefined;
  openedInInnerWorkspace: boolean;
}

interface RecursiveHashEntry {
  entry: CachedFileInfo;
  hashPrefixes: string[];
  importsToInclude: CachedFileInfo[];
}

interface OnCacheMissResult<T> {
  value: T;
  shouldCache: boolean;
}

const enum CacheKey {
  importPaths = 'import_paths',
  emitFile = 'emit_file',
  getDiagnostics = 'get_diagnostics',
  dumpSymbols = 'dump_symbols',
  getInterfaceAST = 'get_interface_ast',
  getEnumAST = 'get_enum_ast',
  getFunctionAST = 'get_function_ast',
}

const CACHE_TTL_MS = 1000 * 60 * 60 * 24 * 30; // 30 days

export class CachingWorkspace implements IWorkspace {
  get initialized(): boolean {
    return this.workspace.initialized;
  }

  get workspaceRoot(): string {
    return this.workspace.workspaceRoot;
  }

  private fileManager: FileManager;
  private fileInfoCache = new Map<string, CachedFileInfo>();
  private taskQueue = new SerialTaskQueue();

  constructor(
    private readonly workspace: IWorkspace,
    private readonly cache: ICompilationCache,
    private readonly logger: ILogger | undefined,
  ) {
    this.fileManager = new FileManager(workspace.workspaceRoot);
  }

  initialize(): void {
    this.workspace.initialize();
    this.cache.evictEntriesBeforeTime(Date.now() - CACHE_TTL_MS);
  }

  destroy(): void {
    this.workspace.destroy();
    this.cache.close();
  }

  private async measureAndLog<T>(command: string, path: string, work: () => Promise<T>): Promise<T> {
    if (!this.logger) {
      return await work();
    }

    const sw = new Stopwatch();
    this.logger?.debug?.(`Begin ${command} on '${path}'`);
    const result = await work();

    this.logger?.debug?.(`End ${command} on '${path}' in ${sw.elapsedString}`);
    return result;
  }

  private invalidateFileInfo(absolutePath: string): void {
    const fileInfo = this.fileInfoCache.get(absolutePath);
    if (fileInfo) {
      this.fileInfoCache.delete(absolutePath);

      if (fileInfo.dependentFiles) {
        for (const dependentFile of fileInfo.dependentFiles) {
          this.invalidateFileInfo(dependentFile);
        }
      }
    }
  }

  registerInMemoryFile(fileName: string, fileContent: string): void {
    const filePath = this.fileManager.resolvePath(fileName);
    this.fileManager.addFileInMemory(filePath, fileContent);

    this.workspace.registerInMemoryFile(fileName, fileContent);

    this.invalidateFileInfo(filePath.absolutePath);
  }

  registerDiskFile(fileName: string, absoluteDiskCachePath: string): void {
    const filePath = this.fileManager.resolvePath(fileName);
    this.fileManager.addFileInDisk(filePath, absoluteDiskCachePath);

    this.workspace.registerDiskFile(fileName, absoluteDiskCachePath);

    this.invalidateFileInfo(filePath.absolutePath);
  }

  async openFile(fileName: string): Promise<OpenFileResult> {
    const cachedFileInfo = this.getCachedFileInfo(fileName);
    const importPaths = await this.resolveImportPaths(cachedFileInfo);

    return {
      importPaths: importPaths.relatives.map((i, index) => {
        return { absolute: importPaths.absolutes[index], relative: i };
      }),
      isNonModuleWithAmbiantDeclarations: importPaths.isNonModuleWithAmbiantDeclarations,
    };
  }

  async emitFile(fileName: string): Promise<EmitResult> {
    return this.processAndCacheRequest(fileName, CacheKey.emitFile, (workspace, fileName) =>
      workspace.emitFile(fileName),
    );
  }

  async getDiagnostics(fileName: string): Promise<GetDiagnosticsResult> {
    return this.processRequest(fileName, CacheKey.getDiagnostics, (workspace, fileName) =>
      workspace.getDiagnostics(fileName).then((value) => {
        const shouldCache = !value.hasError;
        return { shouldCache, value };
      }),
    );
  }

  async dumpSymbolsWithComments(fileName: string): Promise<DumpedSymbolsWithComments> {
    return this.processAndCacheRequest(fileName, CacheKey.dumpSymbols, (workspace, fileName) =>
      workspace.dumpSymbolsWithComments(fileName),
    );
  }

  async getInterfaceAST(fileName: string, position: number): Promise<DumpedInterface> {
    return this.processAndCacheRequest(fileName, `${CacheKey.getInterfaceAST}-${position}`, (workspace, fileName) =>
      workspace.getInterfaceAST(fileName, position),
    );
  }

  async getEnumAST(fileName: string, position: number): Promise<DumpEnumResponseBody> {
    return this.processAndCacheRequest(fileName, `${CacheKey.getEnumAST}-${position}`, (workspace, fileName) =>
      workspace.getEnumAST(fileName, position),
    );
  }

  async getFunctionAST(fileName: string, position: number): Promise<DumpFunctionResponseBody> {
    return this.processAndCacheRequest(fileName, `${CacheKey.getFunctionAST}-${position}`, (workspace, fileName) =>
      workspace.getFunctionAST(fileName, position),
    );
  }

  resolveImportPath(fromPath: string, toPath: string): string {
    return this.workspace.resolveImportPath(fromPath, toPath);
  }

  getFileNameForImportPath(importPath: string): string {
    return this.workspace.getFileNameForImportPath(importPath);
  }

  private processRequest<T>(
    fileName: string,
    cacheKey: string,
    onCacheMiss: (workspace: IWorkspace, fileName: string) => Promise<OnCacheMissResult<T>>,
  ): Promise<T> {
    return this.measureAndLog(cacheKey, fileName, async () => {
      const fileInfo = await this.getResolvedCachedFileInfo(fileName);
      const cachedRequestEntry = await this.getCachedItemForFile(fileInfo, cacheKey);
      if (cachedRequestEntry) {
        return cachedRequestEntry as T;
      }

      await this.openFileInInnerWorkspaceIfNeeded(fileInfo);

      this.logger?.debug?.(`Cache miss for ${cacheKey} on file ${fileName} (recursiveKey: ${fileInfo.recursiveHash})`);

      const newResult = await onCacheMiss(this.workspace, fileName);

      if (newResult.shouldCache) {
        this.setCachedItemForFile(fileInfo, cacheKey, newResult.value);
      }

      return newResult.value;
    });
  }

  private processAndCacheRequest<T>(
    fileName: string,
    cacheKey: string,
    onCacheMiss: (workspace: IWorkspace, fileName: string) => Promise<T>,
  ): Promise<T> {
    return this.processRequest(fileName, cacheKey, (workspace, filename) => {
      return onCacheMiss(workspace, fileName).then((value) => {
        return { shouldCache: true, value };
      });
    });
  }

  private getCachedItemForFile(cacheInfo: CachedFileInfo, cacheKey: string): Promise<unknown | undefined> {
    if (!cacheInfo.recursiveHash) {
      throw new Error('Cannot get a cache entry without resolving the recursive hash first');
    }

    return this.getCachedItemForFileAndHash(cacheInfo, cacheInfo.recursiveHash, cacheKey);
  }

  private async getCachedItemForFileAndHash(
    cacheInfo: CachedFileInfo,
    hash: string,
    cacheKey: string,
  ): Promise<unknown | undefined> {
    try {
      const entry = await this.cache.getCachedEntry(cacheInfo.filePath.absolutePath, cacheKey, hash);
      if (!entry) {
        return undefined;
      }

      return JSON.parse(entry);
    } catch {
      return undefined;
    }
  }

  private setCachedItemForFile(cacheInfo: CachedFileInfo, cacheKey: string, item: unknown): void {
    if (!cacheInfo.recursiveHash) {
      throw new Error('Cannot get a cache entry without resolving the recursive hash first');
    }

    this.setCachedItemForFileAndHash(cacheInfo, cacheInfo.recursiveHash, cacheKey, item);
  }

  private setCachedItemForFileAndHash(cacheInfo: CachedFileInfo, hash: string, cacheKey: string, item: unknown): void {
    const json = JSON.stringify(item);

    try {
      this.cache.setCachedEntry(cacheInfo.filePath.absolutePath, cacheKey, hash, json);
    } catch (err) {
      this.logger?.error?.(`Failed to set cached entry`, err);
    }
  }

  private async getImportPathsFromCache(cacheInfo: CachedFileInfo): Promise<CachedImportPaths | undefined> {
    const importPaths = await this.getCachedItemForFileAndHash(cacheInfo, cacheInfo.shallowHash, CacheKey.importPaths);
    if (!importPaths) {
      return undefined;
    }

    if (typeof importPaths !== 'object') {
      return undefined;
    }

    const cachedImportPaths = importPaths as CachedImportPaths;

    if (
      !Array.isArray(cachedImportPaths.importPaths) ||
      (cachedImportPaths.importPaths.length > 0 && typeof cachedImportPaths.importPaths[0] !== 'object')
    ) {
      return undefined;
    }

    return cachedImportPaths;
  }

  private async doOpenFileInInnerWorkspace(cacheFileInfo: CachedFileInfo): Promise<ImportPaths> {
    const openedFile = await this.workspace.openFile(cacheFileInfo.filePath.absolutePath);
    const importPaths = openedFile.importPaths;
    cacheFileInfo.importPaths = this.makeImportPaths(importPaths, openedFile.isNonModuleWithAmbiantDeclarations);
    cacheFileInfo.openedInInnerWorkspace = true;

    const cachedImportPaths: CachedImportPaths = {
      importPaths,
      isNonModuleWithAmbiantDeclarations: openedFile.isNonModuleWithAmbiantDeclarations,
    };

    this.setCachedItemForFileAndHash(cacheFileInfo, cacheFileInfo.shallowHash, CacheKey.importPaths, cachedImportPaths);

    return cacheFileInfo.importPaths;
  }

  private async openFileInInnerWorkspaceIfNeeded(cacheFileInfo: CachedFileInfo): Promise<void> {
    if (!cacheFileInfo.openedInInnerWorkspace) {
      await this.doOpenFileInInnerWorkspace(cacheFileInfo);
    }
  }

  private makeImportPaths(importPaths: OpenFileImportPath[], isNonModuleWithAmbiantDeclarations: boolean): ImportPaths {
    const relativeImportPaths = importPaths.map((i) => i.relative);
    const absoluteImportPaths = importPaths.map((i) => i.absolute);

    return {
      relatives: relativeImportPaths,
      absolutes: absoluteImportPaths,
      isNonModuleWithAmbiantDeclarations,
    };
  }

  private async resolveImportPaths(cacheFileInfo: CachedFileInfo): Promise<ImportPaths> {
    // First we need to resolve the import paths
    // Use the compilation cache to see if it exists

    if (cacheFileInfo.importPaths) {
      return cacheFileInfo.importPaths;
    }

    const importPaths = await this.getImportPathsFromCache(cacheFileInfo);
    if (importPaths) {
      cacheFileInfo.importPaths = this.makeImportPaths(
        importPaths.importPaths,
        importPaths.isNonModuleWithAmbiantDeclarations,
      );

      if (importPaths.isNonModuleWithAmbiantDeclarations) {
        this.logger?.debug?.(
          `Opening non module file with ambiant declarations ${cacheFileInfo.filePath.absolutePath}`,
        );
      } else {
        return cacheFileInfo.importPaths;
      }
    } else {
      this.logger?.debug?.(
        `Could not resolve import paths from cache ${cacheFileInfo.filePath.absolutePath} (shallowKey: ${cacheFileInfo.shallowHash})`,
      );
    }

    // We don't have import paths for the file.
    // We need to load the file inside the attached workspace
    return this.doOpenFileInInnerWorkspace(cacheFileInfo);
  }

  private async resolveImportedFiles(fileInfo: CachedFileInfo): Promise<CachedFileInfo[]> {
    const output: CachedFileInfo[] = [];
    const absoluteImportPaths = (await this.resolveImportPaths(fileInfo)).absolutes;

    for (const absoluteImportPath of absoluteImportPaths) {
      try {
        const importedFileInfo = this.getCachedFileInfo(absoluteImportPath);
        output.push(importedFileInfo);
      } catch (error: unknown) {
        this.logger?.warn?.(
          `Could not import file ${absoluteImportPath} from ${fileInfo.filePath.absolutePath} (${
            (error as Error).message
          })`,
        );
      }
    }

    return output;
  }

  private computeAndSaveRecursiveHash(
    fileInfo: CachedFileInfo,
    startingHashes: string[],
    importedFiles: CachedFileInfo[],
  ): boolean {
    const allHashes = [...startingHashes];

    for (const importedFile of importedFiles) {
      if (!importedFile.recursiveHash) {
        return false;
      }
      allHashes.push(importedFile.recursiveHash);
    }

    fileInfo.recursiveHash = generateSHA256HashFromStrings(allHashes);
    this.logger?.debug?.(`Resolved recursive hash of ${fileInfo.filePath.absolutePath} as ${fileInfo.recursiveHash}`);

    return true;
  }

  private async gatherFileDependencies(
    fileInfo: CachedFileInfo,
    importedFilesByFileInfo: Map<string, CachedFileInfo[]>,
    circularLoopTracker: CircularLoopTracker,
  ): Promise<void> {
    if (fileInfo.recursiveHash) {
      return;
    }

    switch (circularLoopTracker.push(fileInfo.filePath.absolutePath)) {
      case CircularLoopTrackerPushResult.alreadyVisited:
      case CircularLoopTrackerPushResult.circular:
        return;
      case CircularLoopTrackerPushResult.nonCircular:
        break;
    }

    this.logger?.debug?.(`Resolving recursive hash of ${fileInfo.filePath.absolutePath}`);
    try {
      const importedFiles = await this.resolveImportedFiles(fileInfo);
      importedFilesByFileInfo.set(fileInfo.filePath.absolutePath, importedFiles);

      for (const importedFile of importedFiles) {
        if (!importedFile.dependentFiles) {
          importedFile.dependentFiles = new Set();
        }

        importedFile.dependentFiles.add(fileInfo.filePath.absolutePath);

        await this.gatherFileDependencies(importedFile, importedFilesByFileInfo, circularLoopTracker);
      }
    } catch (err) {
      rethrow(`While gathering dependencies of '${fileInfo.filePath.absolutePath}'`, err);
    } finally {
      circularLoopTracker.pop();
    }
  }

  private makeRecursiveHashEntries(
    fileInfo: CachedFileInfo,
    circularLoopTracker: CircularLoopTracker,
    importedFilesByFileInfo: Map<string, CachedFileInfo[]>,
    seen: Set<string>,
    output: RecursiveHashEntry[],
  ): void {
    const path = fileInfo.filePath.absolutePath;
    if (fileInfo.recursiveHash || seen.has(path)) {
      return;
    }

    seen.add(path);

    const circularEntries = circularLoopTracker.getCircularEntriesForEntry(path);

    const hashPrefixes: string[] = [fileInfo.shallowHash];
    const imports = importedFilesByFileInfo.get(fileInfo.filePath.absolutePath)!;
    let importsToInclude: CachedFileInfo[];

    if (circularEntries) {
      const sortedEntries = [...circularEntries].sort();
      for (const entry of sortedEntries) {
        hashPrefixes.push(this.getCachedFileInfo(entry).shallowHash);
      }
      importsToInclude = [];
      for (const importedFile of imports) {
        if (!circularEntries.has(importedFile.filePath.absolutePath)) {
          importsToInclude.push(importedFile);
        }
      }
    } else {
      importsToInclude = imports;
    }
    output.push({ entry: fileInfo, hashPrefixes, importsToInclude });

    for (const importedFile of imports) {
      this.makeRecursiveHashEntries(importedFile, circularLoopTracker, importedFilesByFileInfo, seen, output);
    }
  }

  private computeRecursiveHashes(entries: RecursiveHashEntry[]): number {
    let entriesToRetry: RecursiveHashEntry[] | undefined;
    let updatedCount = 0;
    for (let i = entries.length; i > 0; i--) {
      const j = i - 1;
      const entry = entries[j];
      if (this.computeAndSaveRecursiveHash(entry.entry, entry.hashPrefixes, entry.importsToInclude)) {
        updatedCount++;
      } else {
        if (!entriesToRetry) {
          entriesToRetry = [];
        }
        entriesToRetry.push(entry);
      }
      entries.pop();
    }

    if (entriesToRetry) {
      for (let i = entriesToRetry.length; i > 0; i--) {
        entries.push(entriesToRetry[i - 1]);
      }
    }

    return updatedCount;
  }

  private async resolveRecursiveHash(fileInfo: CachedFileInfo): Promise<void> {
    const circularLoopTracker = new CircularLoopTracker();
    const importedFilesByFileInfo = new Map<string, CachedFileInfo[]>();
    await this.gatherFileDependencies(fileInfo, importedFilesByFileInfo, circularLoopTracker);

    let recursiveHashEntries: RecursiveHashEntry[] = [];

    this.makeRecursiveHashEntries(
      fileInfo,
      circularLoopTracker,
      importedFilesByFileInfo,
      new Set(),
      recursiveHashEntries,
    );

    while (recursiveHashEntries.length) {
      if (this.computeRecursiveHashes(recursiveHashEntries) === 0) {
        throw new Error('Could not resolve recursive hashes');
      }
    }
  }

  private getResolvedCachedFileInfo(fileName: string): Promise<CachedFileInfo> {
    // We can only resolve one cache file info at a given time.
    // The task queue makes sure that the processing is serial, even if we use async operations
    // underneath.
    return this.taskQueue.enqueueAsyncTask(async () => {
      const cacheFileInfo = this.getCachedFileInfo(fileName);

      if (!cacheFileInfo.recursiveHash) {
        await this.resolveRecursiveHash(cacheFileInfo);
      }

      return cacheFileInfo;
    });
  }

  private resolvePath(fileName: string): FilePath {
    const resolvedFileName = this.workspace.getFileNameForImportPath(fileName) ?? fileName;
    return this.fileManager.resolvePath(resolvedFileName);
  }

  private getCachedFileInfo(fileName: string): CachedFileInfo {
    const resolvedPath = this.resolvePath(fileName);
    const existingCachedFileInfo = this.fileInfoCache.get(resolvedPath.absolutePath);
    if (existingCachedFileInfo) {
      return existingCachedFileInfo;
    }

    const fileContent = this.fileManager.getFile(resolvedPath).content;
    const shallowHash = generateSHA256HashFromString(fileContent);

    this.logger?.debug?.(`Resolved shallow hash of ${resolvedPath.absolutePath} as ${shallowHash}`);

    const cachedFileInfo: CachedFileInfo = {
      shallowHash,
      filePath: resolvedPath,
      recursiveHash: undefined,
      importPaths: undefined,
      dependentFiles: undefined,
      openedInInnerWorkspace: false,
    };
    this.fileInfoCache.set(resolvedPath.absolutePath, cachedFileInfo);

    return cachedFileInfo;
  }
}
