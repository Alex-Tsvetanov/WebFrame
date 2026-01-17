import * as ts from 'typescript';
import * as _path from 'path';
import { Lazy } from './Lazy';
import { FilePath } from '../project/FileManager';

function toAbsoluteImportPath(dirname: string, toPath: string): string {
  if (toPath.startsWith('.') || toPath.startsWith('/')) {
    return _path.resolve(dirname, toPath);
  } else {
    // Absolute import path
    return _path.join('/', toPath);
  }
}

export class ImportPathResolver {
  private resolvedPaths = new Map<string, string>();
  private resolvedFilePathByAlias = new Map<string, FilePath>();
  private didResolvePathsFromCompilerOptions = false;
  private getCompilerOptionsLazy: Lazy<ts.CompilerOptions>;

  constructor(getCompilerOptions: () => ts.CompilerOptions) {
    this.getCompilerOptionsLazy = new Lazy(getCompilerOptions);
  }

  resolveImportPath(fromPath: string, toPath: string): string {
    if (!this.didResolvePathsFromCompilerOptions) {
      this.didResolvePathsFromCompilerOptions = true;
      this.resolvePathsFromCompilerOptions();
    }

    const dirname = _path.dirname(fromPath);
    const resolvedPath = this.resolvedPaths.get(toPath) ?? toPath;
    return toAbsoluteImportPath(dirname, resolvedPath);
  }

  registerAlias(filePath: FilePath): void {
    const fileName = filePath.pathComponents[filePath.pathComponents.length - 1];

    const indexOfDot = fileName.lastIndexOf('.');
    if (indexOfDot < 0) {
      return;
    }

    // Register an alias where the file is available without its extension
    let resolvedAlias = filePath.absolutePath.substring(0, filePath.absolutePath.length - fileName.length + indexOfDot);

    if (resolvedAlias.endsWith('.d')) {
      resolvedAlias = resolvedAlias.substr(0, resolvedAlias.length - 2);
    }

    this.updateAliasIfNeeded(resolvedAlias, filePath);

    if (resolvedAlias.endsWith('/index')) {
      this.updateAliasIfNeeded(resolvedAlias.substring(0, resolvedAlias.length - 6), filePath);
    }
  }

  getAlias(filePath: FilePath): FilePath | undefined {
    return this.resolvedFilePathByAlias.get(filePath.absolutePath);
  }

  private updateAliasIfNeeded(resolvedAlias: string, filePath: FilePath): boolean {
    const existingAlias = this.resolvedFilePathByAlias.get(resolvedAlias);
    if (!existingAlias || (existingAlias.pathExtension === '.js' && filePath.pathExtension !== '.js')) {
      this.resolvedFilePathByAlias.set(resolvedAlias, filePath);
      return true;
    }

    return false;
  }

  private resolvePathsFromCompilerOptions(): void {
    const compilerOptions = this.getCompilerOptionsLazy.target;
    if (!compilerOptions.paths) {
      return;
    }

    const baseUrl = compilerOptions.baseUrl ?? '/';

    const keys = Object.keys(compilerOptions.paths);

    for (const key of keys) {
      const entries = compilerOptions.paths[key]!;
      // TODO(simon): We don't support multiple search path here
      const entry = entries[0];
      const resolvedEntry = toAbsoluteImportPath(baseUrl, entry);

      this.resolvedPaths.set(key, resolvedEntry);
    }
  }
}
