import path = require('path');
import * as ts from 'typescript';
import { FileManager, FilePath } from './FileManager';
import { SourceFileManager } from './SourceFileManager';
import { FileSystemEntries, matchFiles } from './tsInternals';

export const TS_LIBS_VIRTUAL_DIRECTORY = '/__tslibs__';

function throwUnimplemented(parameters: any): never {
  const error = Error(`Methods unimplemented. Parameters given: ${JSON.stringify(parameters, null, 2)}`);

  console.error(error);

  throw error;
}

function getFileSystemEntries(path: FilePath, fileManager: FileManager): FileSystemEntries {
  const files: string[] = [];
  const directories: string[] = [];
  const entries = fileManager.getDirectories(path);

  for (const entry of entries) {
    if (fileManager.fileExists(entry)) {
      files.push(entry.absolutePath);
    } else if (fileManager.directoryExists(entry)) {
      directories.push(entry.absolutePath);
    }
  }

  return { files, directories };
}

class HostBase implements ts.FormatDiagnosticsHost {
  constructor(readonly fileManager: FileManager) {}

  getCurrentDirectory(): string {
    return this.fileManager.getCurrentDirectory();
  }

  getNewLine(): string {
    return '\n';
  }

  getCanonicalFileName(fileName: string): string {
    return this.fileManager.resolvePath(fileName).absolutePath;
  }

  readDirectory(
    rootDir: string,
    extensions: readonly string[],
    excludes: readonly string[] | undefined,
    includes: readonly string[],
    depth?: number,
  ): string[] {
    const currentDirectory = this.fileManager.getCurrentDirectory();

    const output = matchFiles(
      rootDir,
      extensions,
      excludes || [],
      includes,
      true,
      currentDirectory,
      depth,
      (path) => {
        // const includeDir = dirPathMatches(path);
        const standardizedPath = this.fileManager.resolvePath(path);
        return getFileSystemEntries(standardizedPath, this.fileManager);
      },
      (path) => this.fileManager.resolvePath(path).absolutePath,
      (path) => this.fileManager.directoryExists(this.fileManager.resolvePath(path)),
    );

    return output;
  }

  getDirectories?(directoryName: string): string[] {
    return this.fileManager.getDirectories(this.fileManager.resolvePath(directoryName)).map((p) => p.absolutePath);
  }

  realpath?(path: string): string {
    return this.fileManager.resolvePath(path).absolutePath;
  }

  directoryExists?(directoryName: string): boolean {
    if (directoryName.endsWith('node_modules/@types')) {
      return false;
    }

    return this.fileManager.directoryExists(this.fileManager.resolvePath(directoryName));
  }

  fileExists(path: string): boolean {
    return this.fileManager.fileExists(this.fileManager.resolvePath(path));
  }

  readFile(path: string): string | undefined {
    const resolvedPath = this.fileManager.resolvePath(path);
    try {
      return this.fileManager.getFile(resolvedPath).content;
    } catch {
      return undefined;
    }
  }
}

class SourceFileHostBase extends HostBase {
  constructor(fileManager: FileManager, readonly sourceFileManager: SourceFileManager) {
    super(fileManager);
  }

  getDefaultLibFileName(options: ts.CompilerOptions): string {
    return path.resolve(TS_LIBS_VIRTUAL_DIRECTORY, ts.getDefaultLibFileName(options));
  }

  useCaseSensitiveFileNames(): boolean {
    return true;
  }
}

export class ParseConfigHost extends HostBase implements ts.ParseConfigHost {
  useCaseSensitiveFileNames = true;
}

export class CompilerHost extends SourceFileHostBase implements ts.CompilerHost {
  constructor(fileManager: FileManager, sourceFileManager: SourceFileManager) {
    super(fileManager, sourceFileManager);
  }
  getSourceFile(
    fileName: string,
    languageVersionOrOptions: ts.ScriptTarget | ts.CreateSourceFileOptions,
    onError?: ((message: string) => void) | undefined,
    shouldCreateNewSourceFile?: boolean | undefined,
  ): ts.SourceFile | undefined {
    return this.sourceFileManager.getOrCreateSourceFile(fileName, languageVersionOrOptions);
  }

  writeFile: ts.WriteFileCallback = (
    fileName: string,
    text: string,
    writeByteOrderMark: boolean,
    onError?: (message: string) => void,
    sourceFiles?: readonly ts.SourceFile[],
    data?: ts.WriteFileCallbackData,
  ) => {
    throwUnimplemented({ fileName, text, writeByteOrderMark });
  };

  getEnvironmentVariable?(name: string): string | undefined {
    return process.env[name];
  }
}

export class FormatDiagnosticsHost extends HostBase implements ts.FormatDiagnosticsHost {}
