import 'ts-jest';
import { EmitResult, GetDiagnosticsResult, IWorkspace, OpenFileResult } from '../IWorkspace';
import {
  DumpEnumResponseBody,
  DumpFunctionResponseBody,
  DumpedInterface,
  DumpedSymbolsWithComments,
} from '../protocol';
import { ICompilationCache } from './ICompilationCache';
import { CachingWorkspace } from './CachingWorkspace';
import { ImportPathResolver } from '../utils/ImportPathResolver';
import { FileManager } from '../project/FileManager';

interface TestWorkspaceResponseHandler {
  method: string;
  fileName: string;
  position: number | undefined;
  response: unknown;
}

class TestWorkspace implements IWorkspace {
  initialized: boolean = true;
  workspaceRoot: string = '/';

  responseHandlers: TestWorkspaceResponseHandler[] = [];
  private fileManager = new FileManager(this.workspaceRoot);
  private importPathResolver = new ImportPathResolver(() => {
    return {};
  });

  initialize(): void {}

  destroy(): void {}

  registerInMemoryFile(fileName: string, fileContent: string): void {
    this.importPathResolver.registerAlias(this.fileManager.resolvePath(fileName));
  }

  registerDiskFile(fileName: string, absoluteDiskPath: string): void {
    this.importPathResolver.registerAlias(this.fileManager.resolvePath(fileName));
  }

  resolveImportPath(fromPath: string, toPath: string): string {
    return this.importPathResolver.resolveImportPath(fromPath, toPath);
  }

  getFileNameForImportPath(importPath: string): string {
    return this.importPathResolver.getAlias(this.fileManager.resolvePath(importPath))?.absolutePath ?? importPath;
  }

  appendNextRequestHandler<T>(method: string, fileName: string, position: number | undefined, response: T): void {
    this.responseHandlers.push({ method, fileName, position, response });
  }

  private async handleRequest<T>(method: string, fileName: string, position?: number): Promise<T> {
    for (let i = 0; i < this.responseHandlers.length; i++) {
      const responseHandler = this.responseHandlers[i];
      if (
        responseHandler.method === method &&
        responseHandler.fileName === fileName &&
        responseHandler.position === position
      ) {
        const response = responseHandler.response;
        this.responseHandlers.splice(i, 1);
        return response as T;
      }
    }

    throw new Error(`No request handler for method '${method}' with fileName '${fileName}'`);
  }

  openFile(fileName: string): Promise<OpenFileResult> {
    return this.handleRequest<OpenFileResult>('openFile', fileName);
  }

  emitFile(fileName: string): Promise<EmitResult> {
    return this.handleRequest<EmitResult>('emitFile', fileName);
  }

  getDiagnostics(fileName: string): Promise<GetDiagnosticsResult> {
    return this.handleRequest<GetDiagnosticsResult>('getDiagnostics', fileName);
  }

  dumpSymbolsWithComments(fileName: string): Promise<DumpedSymbolsWithComments> {
    return this.handleRequest<DumpedSymbolsWithComments>('dumpSymbolsWithComments', fileName);
  }

  getInterfaceAST(fileName: string, position: number): Promise<DumpedInterface> {
    return this.handleRequest<DumpedInterface>('getInterfaceAST', fileName, position);
  }

  getEnumAST(fileName: string, position: number): Promise<DumpEnumResponseBody> {
    return this.handleRequest<DumpEnumResponseBody>('getEnumAST', fileName, position);
  }

  getFunctionAST(fileName: string, position: number): Promise<DumpFunctionResponseBody> {
    return this.handleRequest<DumpFunctionResponseBody>('getFunctionAST', fileName, position);
  }
}

interface TestCompilationCacheEntry {
  container: string;
  hash: string;
  data: string;
}

class TestCompilationCache implements ICompilationCache {
  entriesByPath: { [key: string]: TestCompilationCacheEntry[] } = {};

  private doResolveEntry(path: string, container: string, hash: string): TestCompilationCacheEntry | undefined {
    const entries = this.entriesByPath[path];
    if (!entries) {
      return undefined;
    }
    for (const entry of entries) {
      if (entry.container === container) {
        if (entry.hash === hash) {
          return entry;
        } else {
          return undefined;
        }
      }
    }

    return undefined;
  }

  close(): void {}

  evictEntriesBeforeTime(lastUsedTimestamp: number): void {}

  async getCachedEntry(path: string, container: string, hash: string): Promise<string | undefined> {
    return this.doResolveEntry(path, container, hash)?.data;
  }

  async setCachedEntry(path: string, container: string, hash: string, data: string): Promise<void> {
    let entry = this.doResolveEntry(path, container, hash);
    if (!entry) {
      entry = { container, hash, data };
      let entries = this.entriesByPath[path];
      if (!entries) {
        entries = [];
        this.entriesByPath[path] = entries;
      }
      entries.push(entry);
    } else {
      entry.data = data;
    }
  }
}

describe('CachingWorkspace', () => {
  it('stores open file in cache', async () => {
    const innerWorkspace = new TestWorkspace();
    const cache = new TestCompilationCache();

    let workspace = new CachingWorkspace(innerWorkspace, cache, undefined);
    workspace.registerInMemoryFile('Foo.ts', 'Hello World');

    innerWorkspace.appendNextRequestHandler<OpenFileResult>('openFile', '/Foo.ts', undefined, {
      importPaths: [{ relative: 'Bar', absolute: '/Bar' }],
      isNonModuleWithAmbiantDeclarations: false,
    });

    let result = await workspace.openFile('Foo.ts');

    expect(result.importPaths).toEqual([{ relative: 'Bar', absolute: '/Bar' }]);

    // now recreate the workspace and perform the request again. It should hit the cache
    workspace = new CachingWorkspace(innerWorkspace, cache, undefined);
    workspace.registerInMemoryFile('Foo.ts', 'Hello World');

    result = await workspace.openFile('Foo.ts');

    expect(result.importPaths).toEqual([{ relative: 'Bar', absolute: '/Bar' }]);

    // Recreate the workspace and change the file content. It should not hit the cache.
    workspace = new CachingWorkspace(innerWorkspace, cache, undefined);
    workspace.registerInMemoryFile('Foo.ts', 'Goodbye World');

    innerWorkspace.appendNextRequestHandler<OpenFileResult>('openFile', '/Foo.ts', undefined, {
      importPaths: [{ relative: 'Bar2', absolute: '/Bar2' }],
      isNonModuleWithAmbiantDeclarations: false,
    });

    result = await workspace.openFile('Foo.ts');

    expect(result.importPaths).toEqual([{ relative: 'Bar2', absolute: '/Bar2' }]);
  });

  it('uses dependent files to compute hash', async () => {
    const innerWorkspace = new TestWorkspace();
    const cache = new TestCompilationCache();

    let workspace = new CachingWorkspace(innerWorkspace, cache, undefined);
    workspace.registerInMemoryFile('Foo.ts', 'A');
    workspace.registerInMemoryFile('Bar.ts', 'B');
    workspace.registerInMemoryFile('Last.ts', 'C');

    innerWorkspace.appendNextRequestHandler<OpenFileResult>('openFile', '/Foo.ts', undefined, {
      importPaths: [{ relative: 'Bar', absolute: '/Bar' }],
      isNonModuleWithAmbiantDeclarations: false,
    });

    innerWorkspace.appendNextRequestHandler<OpenFileResult>('openFile', '/Bar.ts', undefined, {
      importPaths: [{ relative: 'Last', absolute: '/Last' }],
      isNonModuleWithAmbiantDeclarations: false,
    });

    innerWorkspace.appendNextRequestHandler<OpenFileResult>('openFile', '/Last.ts', undefined, {
      importPaths: [],
      isNonModuleWithAmbiantDeclarations: false,
    });

    await workspace.openFile('Foo.ts');

    // We should not have asked to open Bar.ts and Last.ts yet
    expect(innerWorkspace.responseHandlers.length).toBe(2);
    expect(cache.entriesByPath).toEqual({
      '/Foo.ts': [
        {
          container: 'import_paths',
          hash: '559aead08264d5795d3909718cdd05abd49572e84fe55590eef31a88a08fdffd',
          data: '{"importPaths":[{"relative":"Bar","absolute":"/Bar"}],"isNonModuleWithAmbiantDeclarations":false}',
        },
      ],
    });

    innerWorkspace.appendNextRequestHandler<EmitResult>('emitFile', '/Foo.ts', undefined, {
      entries: [],
      emitted: true,
    });

    // Now we ask to emit the file

    let result = await workspace.emitFile('/Foo.ts');
    expect(result).toEqual({
      entries: [],
      emitted: true,
    });

    // All the request handlers should have been exhausted. It should have requested
    // to open all the dependent files
    expect(innerWorkspace.responseHandlers.length).toBe(0);
    expect(cache.entriesByPath).toEqual({
      '/Foo.ts': [
        {
          container: 'import_paths',
          hash: '559aead08264d5795d3909718cdd05abd49572e84fe55590eef31a88a08fdffd',
          data: '{"importPaths":[{"relative":"Bar","absolute":"/Bar"}],"isNonModuleWithAmbiantDeclarations":false}',
        },
        {
          container: 'emit_file',
          hash: 'a06f2a8faa0695f12035b9ca694f1a430aa2a4f374f1639ed218db2d2962acfe',
          data: '{"entries":[],"emitted":true}',
        },
      ],
      '/Bar.ts': [
        {
          container: 'import_paths',
          hash: 'df7e70e5021544f4834bbee64a9e3789febc4be81470df629cad6ddb03320a5c',
          data: '{"importPaths":[{"relative":"Last","absolute":"/Last"}],"isNonModuleWithAmbiantDeclarations":false}',
        },
      ],
      '/Last.ts': [
        {
          container: 'import_paths',
          hash: '6b23c0d5f35d1b11f9b683f0b0a617355deb11277d91ae091d399c655b87940d',
          data: '{"importPaths":[],"isNonModuleWithAmbiantDeclarations":false}',
        },
      ],
    });

    // Re-create workspace with same files
    workspace = new CachingWorkspace(innerWorkspace, cache, undefined);
    workspace.registerInMemoryFile('Foo.ts', 'A');
    workspace.registerInMemoryFile('Bar.ts', 'B');
    workspace.registerInMemoryFile('Last.ts', 'C');

    await workspace.openFile('Foo.ts');
    // Should be able to retrieve info from cache
    result = await workspace.emitFile('/Foo.ts');
    expect(result).toEqual({
      entries: [],
      emitted: true,
    });

    // Re-create workspace and change Last.ts content
    workspace = new CachingWorkspace(innerWorkspace, cache, undefined);
    workspace.registerInMemoryFile('Foo.ts', 'A');
    workspace.registerInMemoryFile('Bar.ts', 'B');
    workspace.registerInMemoryFile('Last.ts', 'D');

    await workspace.openFile('Foo.ts');

    innerWorkspace.appendNextRequestHandler<OpenFileResult>('openFile', '/Foo.ts', undefined, {
      importPaths: [{ relative: 'Bar', absolute: '/Bar' }],
      isNonModuleWithAmbiantDeclarations: false,
    });

    innerWorkspace.appendNextRequestHandler<OpenFileResult>('openFile', '/Last.ts', undefined, {
      importPaths: [],
      isNonModuleWithAmbiantDeclarations: false,
    });

    innerWorkspace.appendNextRequestHandler<EmitResult>('emitFile', '/Foo.ts', undefined, {
      entries: [],
      emitted: true,
    });

    result = await workspace.emitFile('/Foo.ts');

    expect(cache.entriesByPath).toEqual({
      '/Foo.ts': [
        {
          container: 'import_paths',
          hash: '559aead08264d5795d3909718cdd05abd49572e84fe55590eef31a88a08fdffd',
          data: '{"importPaths":[{"relative":"Bar","absolute":"/Bar"}],"isNonModuleWithAmbiantDeclarations":false}',
        },
        {
          container: 'emit_file',
          hash: 'a06f2a8faa0695f12035b9ca694f1a430aa2a4f374f1639ed218db2d2962acfe',
          data: '{"entries":[],"emitted":true}',
        },
        {
          data: '{"entries":[],"emitted":true}',
          hash: 'a5d0cb5e3d68c7dc768033126974223f7a6fb4a3ed67821cc41436d93b76300c',
          container: 'emit_file',
        },
      ],
      '/Bar.ts': [
        {
          container: 'import_paths',
          hash: 'df7e70e5021544f4834bbee64a9e3789febc4be81470df629cad6ddb03320a5c',
          data: '{"importPaths":[{"relative":"Last","absolute":"/Last"}],"isNonModuleWithAmbiantDeclarations":false}',
        },
      ],
      '/Last.ts': [
        {
          container: 'import_paths',
          hash: '6b23c0d5f35d1b11f9b683f0b0a617355deb11277d91ae091d399c655b87940d',
          data: '{"importPaths":[],"isNonModuleWithAmbiantDeclarations":false}',
        },
        {
          data: '{"importPaths":[],"isNonModuleWithAmbiantDeclarations":false}',
          hash: '3f39d5c348e5b79d06e842c114e6cc571583bbf44e4b0ebfda1a01ec05745d43',
          container: 'import_paths',
        },
      ],
    });
  });

  it('resolves relative import paths', async () => {
    const innerWorkspace = new TestWorkspace();
    const cache = new TestCompilationCache();

    let workspace = new CachingWorkspace(innerWorkspace, cache, undefined);
    workspace.registerInMemoryFile('dir/a/Foo.ts', 'A');
    workspace.registerInMemoryFile('other/b/Bar.ts', 'B');
    workspace.registerInMemoryFile('other/b/Last.ts', 'C');

    innerWorkspace.appendNextRequestHandler<OpenFileResult>('openFile', '/dir/a/Foo.ts', undefined, {
      importPaths: [{ relative: '../../other/b/Bar', absolute: '/other/b/Bar' }],
      isNonModuleWithAmbiantDeclarations: false,
    });

    innerWorkspace.appendNextRequestHandler<OpenFileResult>('openFile', '/other/b/Bar.ts', undefined, {
      importPaths: [{ relative: './Last', absolute: '/other/b/Last' }],
      isNonModuleWithAmbiantDeclarations: false,
    });

    innerWorkspace.appendNextRequestHandler<OpenFileResult>('openFile', '/other/b/Last.ts', undefined, {
      importPaths: [],
      isNonModuleWithAmbiantDeclarations: false,
    });

    innerWorkspace.appendNextRequestHandler<EmitResult>('emitFile', '/dir/a/Foo.ts', undefined, {
      entries: [],
      emitted: true,
    });

    await workspace.openFile('dir/a/Foo.ts');
    const result = await workspace.emitFile('/dir/a/Foo.ts');
    expect(result).toEqual({
      entries: [],
      emitted: true,
    });

    expect(cache.entriesByPath).toEqual({
      '/dir/a/Foo.ts': [
        {
          container: 'import_paths',
          hash: '559aead08264d5795d3909718cdd05abd49572e84fe55590eef31a88a08fdffd',
          data: '{"importPaths":[{"relative":"../../other/b/Bar","absolute":"/other/b/Bar"}],"isNonModuleWithAmbiantDeclarations":false}',
        },
        {
          container: 'emit_file',
          hash: 'a06f2a8faa0695f12035b9ca694f1a430aa2a4f374f1639ed218db2d2962acfe',
          data: '{"entries":[],"emitted":true}',
        },
      ],
      '/other/b/Bar.ts': [
        {
          container: 'import_paths',
          hash: 'df7e70e5021544f4834bbee64a9e3789febc4be81470df629cad6ddb03320a5c',
          data: '{"importPaths":[{"relative":"./Last","absolute":"/other/b/Last"}],"isNonModuleWithAmbiantDeclarations":false}',
        },
      ],
      '/other/b/Last.ts': [
        {
          container: 'import_paths',
          hash: '6b23c0d5f35d1b11f9b683f0b0a617355deb11277d91ae091d399c655b87940d',
          data: '{"importPaths":[],"isNonModuleWithAmbiantDeclarations":false}',
        },
      ],
    });
  });

  it('resolves absolute import paths', async () => {
    const innerWorkspace = new TestWorkspace();
    const cache = new TestCompilationCache();

    let workspace = new CachingWorkspace(innerWorkspace, cache, undefined);
    workspace.registerInMemoryFile('dir/a/Foo.ts', 'A');
    workspace.registerInMemoryFile('other/b/Bar.ts', 'B');

    innerWorkspace.appendNextRequestHandler<OpenFileResult>('openFile', '/dir/a/Foo.ts', undefined, {
      importPaths: [{ relative: 'other/b/Bar', absolute: '/other/b/Bar' }],
      isNonModuleWithAmbiantDeclarations: false,
    });

    innerWorkspace.appendNextRequestHandler<OpenFileResult>('openFile', '/other/b/Bar.ts', undefined, {
      importPaths: [],
      isNonModuleWithAmbiantDeclarations: false,
    });

    innerWorkspace.appendNextRequestHandler<EmitResult>('emitFile', '/dir/a/Foo.ts', undefined, {
      entries: [],
      emitted: true,
    });

    await workspace.openFile('dir/a/Foo.ts');
    const result = await workspace.emitFile('/dir/a/Foo.ts');
    expect(result).toEqual({
      entries: [],
      emitted: true,
    });

    expect(cache.entriesByPath).toEqual({
      '/dir/a/Foo.ts': [
        {
          container: 'import_paths',
          hash: '559aead08264d5795d3909718cdd05abd49572e84fe55590eef31a88a08fdffd',
          data: '{"importPaths":[{"relative":"other/b/Bar","absolute":"/other/b/Bar"}],"isNonModuleWithAmbiantDeclarations":false}',
        },
        {
          container: 'emit_file',
          hash: '5b5554131b95d5d7362050de7fe6545c94927b23dde06a0a7565d6d82d6a526c',
          data: '{"entries":[],"emitted":true}',
        },
      ],
      '/other/b/Bar.ts': [
        {
          container: 'import_paths',
          hash: 'df7e70e5021544f4834bbee64a9e3789febc4be81470df629cad6ddb03320a5c',
          data: '{"importPaths":[],"isNonModuleWithAmbiantDeclarations":false}',
        },
      ],
    });
  });

  it('resolves definition ts files', async () => {
    const innerWorkspace = new TestWorkspace();
    const cache = new TestCompilationCache();

    let workspace = new CachingWorkspace(innerWorkspace, cache, undefined);
    workspace.registerInMemoryFile('Foo.ts', 'A');
    workspace.registerInMemoryFile('Bar.d.ts', 'B');

    innerWorkspace.appendNextRequestHandler<OpenFileResult>('openFile', '/Foo.ts', undefined, {
      importPaths: [{ relative: 'Bar', absolute: '/Bar' }],
      isNonModuleWithAmbiantDeclarations: false,
    });

    innerWorkspace.appendNextRequestHandler<OpenFileResult>('openFile', '/Bar.d.ts', undefined, {
      importPaths: [],
      isNonModuleWithAmbiantDeclarations: false,
    });

    innerWorkspace.appendNextRequestHandler<EmitResult>('emitFile', '/Foo.ts', undefined, {
      entries: [],
      emitted: true,
    });

    await workspace.openFile('Foo.ts');
    const result = await workspace.emitFile('/Foo.ts');
    expect(result).toEqual({
      entries: [],
      emitted: true,
    });

    expect(cache.entriesByPath).toEqual({
      '/Foo.ts': [
        {
          container: 'import_paths',
          hash: '559aead08264d5795d3909718cdd05abd49572e84fe55590eef31a88a08fdffd',
          data: '{"importPaths":[{"relative":"Bar","absolute":"/Bar"}],"isNonModuleWithAmbiantDeclarations":false}',
        },
        {
          container: 'emit_file',
          hash: '5b5554131b95d5d7362050de7fe6545c94927b23dde06a0a7565d6d82d6a526c',
          data: '{"entries":[],"emitted":true}',
        },
      ],
      '/Bar.d.ts': [
        {
          container: 'import_paths',
          hash: 'df7e70e5021544f4834bbee64a9e3789febc4be81470df629cad6ddb03320a5c',
          data: '{"importPaths":[],"isNonModuleWithAmbiantDeclarations":false}',
        },
      ],
    });
  });

  it('supports circular imports', async () => {
    const innerWorkspace = new TestWorkspace();
    const cache = new TestCompilationCache();

    let workspace = new CachingWorkspace(innerWorkspace, cache, undefined);
    workspace.registerInMemoryFile('Foo.ts', 'A');
    workspace.registerInMemoryFile('Bar.ts', 'B');

    innerWorkspace.appendNextRequestHandler<OpenFileResult>('openFile', '/Foo.ts', undefined, {
      importPaths: [{ relative: './Bar', absolute: '/Bar' }],
      isNonModuleWithAmbiantDeclarations: false,
    });

    innerWorkspace.appendNextRequestHandler<OpenFileResult>('openFile', '/Bar.ts', undefined, {
      importPaths: [{ relative: './Foo', absolute: '/Foo' }],
      isNonModuleWithAmbiantDeclarations: false,
    });

    await workspace.openFile('Foo.ts');

    innerWorkspace.appendNextRequestHandler<EmitResult>('emitFile', '/Foo.ts', undefined, {
      entries: [],
      emitted: true,
    });
    innerWorkspace.appendNextRequestHandler<EmitResult>('emitFile', '/Bar.ts', undefined, {
      entries: [],
      emitted: true,
    });

    await workspace.emitFile('/Foo.ts');
    await workspace.emitFile('/Bar.ts');

    expect(innerWorkspace.responseHandlers.length).toBe(0);

    // Invalidating Foo should invalidate both files
    workspace = new CachingWorkspace(innerWorkspace, cache, undefined);
    workspace.registerInMemoryFile('Foo.ts', 'C');
    workspace.registerInMemoryFile('Bar.ts', 'B');

    innerWorkspace.appendNextRequestHandler<OpenFileResult>('openFile', '/Foo.ts', undefined, {
      importPaths: [{ relative: './Bar', absolute: '/Bar' }],
      isNonModuleWithAmbiantDeclarations: false,
    });

    innerWorkspace.appendNextRequestHandler<OpenFileResult>('openFile', '/Bar.ts', undefined, {
      importPaths: [{ relative: './Foo', absolute: '/Foo' }],
      isNonModuleWithAmbiantDeclarations: false,
    });

    innerWorkspace.appendNextRequestHandler<EmitResult>('emitFile', '/Foo.ts', undefined, {
      entries: [],
      emitted: true,
    });
    innerWorkspace.appendNextRequestHandler<EmitResult>('emitFile', '/Bar.ts', undefined, {
      entries: [],
      emitted: true,
    });

    await workspace.openFile('Foo.ts');
    await workspace.emitFile('/Foo.ts');
    await workspace.emitFile('/Bar.ts');

    expect(innerWorkspace.responseHandlers.length).toBe(0);

    // Invalidating Bar should invalidate both files as well
    workspace = new CachingWorkspace(innerWorkspace, cache, undefined);
    workspace.registerInMemoryFile('Foo.ts', 'C');
    workspace.registerInMemoryFile('Bar.ts', 'D');

    innerWorkspace.appendNextRequestHandler<OpenFileResult>('openFile', '/Foo.ts', undefined, {
      importPaths: [{ relative: './Bar', absolute: '/Bar' }],
      isNonModuleWithAmbiantDeclarations: false,
    });

    innerWorkspace.appendNextRequestHandler<OpenFileResult>('openFile', '/Bar.ts', undefined, {
      importPaths: [{ relative: './Foo', absolute: '/Foo' }],
      isNonModuleWithAmbiantDeclarations: false,
    });

    innerWorkspace.appendNextRequestHandler<EmitResult>('emitFile', '/Foo.ts', undefined, {
      entries: [],
      emitted: true,
    });
    innerWorkspace.appendNextRequestHandler<EmitResult>('emitFile', '/Bar.ts', undefined, {
      entries: [],
      emitted: true,
    });

    await workspace.openFile('Bar.ts');
    await workspace.emitFile('/Foo.ts');
    await workspace.emitFile('/Bar.ts');

    expect(innerWorkspace.responseHandlers.length).toBe(0);
  });

  it('invalidates files on hot reload', async () => {
    const innerWorkspace = new TestWorkspace();
    const cache = new TestCompilationCache();

    let workspace = new CachingWorkspace(innerWorkspace, cache, undefined);
    workspace.registerInMemoryFile('Foo.ts', 'A');
    workspace.registerInMemoryFile('Bar.ts', 'B');
    workspace.registerInMemoryFile('Last.ts', 'C');

    innerWorkspace.appendNextRequestHandler<OpenFileResult>('openFile', '/Foo.ts', undefined, {
      importPaths: [{ relative: 'Bar', absolute: '/Bar' }],
      isNonModuleWithAmbiantDeclarations: false,
    });

    innerWorkspace.appendNextRequestHandler<OpenFileResult>('openFile', '/Bar.ts', undefined, {
      importPaths: [{ relative: 'Last', absolute: '/Last' }],
      isNonModuleWithAmbiantDeclarations: false,
    });

    innerWorkspace.appendNextRequestHandler<OpenFileResult>('openFile', '/Last.ts', undefined, {
      importPaths: [],
      isNonModuleWithAmbiantDeclarations: false,
    });

    await workspace.openFile('Foo.ts');

    innerWorkspace.appendNextRequestHandler<EmitResult>('emitFile', '/Foo.ts', undefined, {
      entries: [],
      emitted: true,
    });

    // Now we ask to emit the file

    await workspace.emitFile('/Foo.ts');

    expect(innerWorkspace.responseHandlers.length).toBe(0);

    // Invalidate with same files
    workspace.registerInMemoryFile('Foo.ts', 'A');
    workspace.registerInMemoryFile('Bar.ts', 'B');
    workspace.registerInMemoryFile('Last.ts', 'C');

    await workspace.openFile('Foo.ts');
    // Should be able to retrieve info from cache
    await workspace.emitFile('/Foo.ts');

    // Change Last.ts content
    workspace.registerInMemoryFile('Last.ts', 'D');

    await workspace.openFile('Foo.ts');

    innerWorkspace.appendNextRequestHandler<OpenFileResult>('openFile', '/Foo.ts', undefined, {
      importPaths: [{ relative: 'Bar.ts', absolute: '/Bar.ts' }],
      isNonModuleWithAmbiantDeclarations: false,
    });

    innerWorkspace.appendNextRequestHandler<OpenFileResult>('openFile', '/Last.ts', undefined, {
      importPaths: [],
      isNonModuleWithAmbiantDeclarations: false,
    });

    innerWorkspace.appendNextRequestHandler<EmitResult>('emitFile', '/Foo.ts', undefined, {
      entries: [],
      emitted: true,
    });

    await workspace.emitFile('/Foo.ts');

    expect(innerWorkspace.responseHandlers.length).toBe(0);
  });

  it('always open file with ambiant declarations in nested workspace', async () => {
    const innerWorkspace = new TestWorkspace();
    const cache = new TestCompilationCache();

    let workspace = new CachingWorkspace(innerWorkspace, cache, undefined);
    workspace.registerInMemoryFile('Foo.ts', 'Hello World');

    innerWorkspace.appendNextRequestHandler<OpenFileResult>('openFile', '/Foo.ts', undefined, {
      importPaths: [],
      isNonModuleWithAmbiantDeclarations: true,
    });

    await workspace.openFile('Foo.ts');

    expect(innerWorkspace.responseHandlers.length).toEqual(0);

    // now recreate the workspace and perform the request again. It should hit the inner workspace
    // since isNonModuleWithAmbiantDeclarations is true
    workspace = new CachingWorkspace(innerWorkspace, cache, undefined);
    workspace.registerInMemoryFile('Foo.ts', 'Hello World');

    innerWorkspace.appendNextRequestHandler<OpenFileResult>('openFile', '/Foo.ts', undefined, {
      importPaths: [],
      isNonModuleWithAmbiantDeclarations: true,
    });

    await workspace.openFile('Foo.ts');

    expect(innerWorkspace.responseHandlers.length).toEqual(0);
  });
});
