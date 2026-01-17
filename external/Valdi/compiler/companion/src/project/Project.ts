import * as ts from 'typescript';
import { FileManager, FilePath, IFileManagerListener } from './FileManager';
import { CompilerHost, FormatDiagnosticsHost, ParseConfigHost, TS_LIBS_VIRTUAL_DIRECTORY } from './TypeScriptHosts';
import { ISourceFileManagerListener, SourceFileManager } from './SourceFileManager';
import { TSLibsDatabase } from '../tslibs/TSLibsDatabase';
import * as path from 'path';

export interface IProjectListener {
  onProgramWillChange(): void;
  onProgramChanged(): void;
  onCompilerOptionsResolved(): void;
  onFileContentLoaded(path: string): void;
  onSourceFileCreated(path: string): void;
  onSourceFileInvalidated(path: string): void;
}

export class Project implements ISourceFileManagerListener, IFileManagerListener {
  get typeChecker(): ts.TypeChecker {
    return this.program.getTypeChecker();
  }

  get program(): ts.Program {
    return this.loadProgramIfNeeded();
  }

  get compilerOptions(): ts.CompilerOptions {
    if (!this.resolvedCompilerOptions) {
      this.resolvedCompilerOptions = this.resolveCompilerOptions();
      this.listener?.onCompilerOptionsResolved();
    }
    return this.resolvedCompilerOptions;
  }

  private currentProgram: ts.Program | undefined = undefined;
  private resolvedCompilerOptions: ts.CompilerOptions | undefined = undefined;

  private fileManager: FileManager;
  private sourceFileManager: SourceFileManager;
  private programDirty = false;
  private updatingProgram = false;

  constructor(readonly rootPath: string, compilerOptions?: ts.CompilerOptions, readonly listener?: IProjectListener) {
    this.fileManager = new FileManager(rootPath, this);
    this.sourceFileManager = new SourceFileManager(this.fileManager, this);

    this.resolvedCompilerOptions = compilerOptions;

    // Populate TSLibs
    const database = TSLibsDatabase.get();
    for (const entry of database.entries) {
      const resolvedPath = this.fileManager.resolvePath(path.join(TS_LIBS_VIRTUAL_DIRECTORY, entry.fileName));
      this.fileManager.addFileInMemory(resolvedPath, entry.content);
    }
  }

  destroy(): void {
    this.fileManager.clear();
    this.sourceFileManager.clear();
    this.currentProgram = undefined;
    this.resolvedCompilerOptions = undefined;
  }

  fileExists(path: string): boolean {
    return this.fileManager.fileExists(this.fileManager.resolvePath(path));
  }

  createSourceFile(path: string, content: string): ts.SourceFile {
    this.setFileContentAtPath(path, content);
    return this.openSourceFile(path);
  }

  openSourceFile(path: string): ts.SourceFile {
    return this.sourceFileManager.getOrCreateSourceFile(path, this.compilerOptions.target ?? ts.ScriptTarget.Latest);
  }

  getOpenedSourceFile(path: string): ts.SourceFile | undefined {
    return this.sourceFileManager.getSourceFile(path);
  }

  removeSourceFile(path: string) {
    this.sourceFileManager.removeSourceFile(path);
  }

  resolvePath(path: string): FilePath {
    return this.fileManager.resolvePath(path);
  }

  setFileContentAtPath(path: string, content: string): void {
    this.setFileContentAtResolvedPath(this.resolvePath(path), content);
  }

  setFileFromDiskAtPath(path: string, absoluteDiskPath: string): void {
    this.setFileFromDiskAtResolvedPath(this.resolvePath(path), absoluteDiskPath);
  }

  setFileContentAtResolvedPath(resolvedPath: FilePath, content: string): void {
    this.fileManager.addFileInMemory(resolvedPath, content);
    this.sourceFileManager.invalidateSourceFile(resolvedPath);
  }

  setFileFromDiskAtResolvedPath(resolvedPath: FilePath, absoluteDiskPath: string): void {
    this.fileManager.addFileInDisk(resolvedPath, absoluteDiskPath);
    this.sourceFileManager.invalidateSourceFile(resolvedPath);
  }

  formatDiagnostics(diagnostics: readonly ts.Diagnostic[]): string {
    return ts.formatDiagnostics(diagnostics, new FormatDiagnosticsHost(this.fileManager));
  }

  formatDiagnostic(diagnostic: ts.Diagnostic): string {
    return ts.formatDiagnostic(diagnostic, new FormatDiagnosticsHost(this.fileManager));
  }

  onSourceFileCreated(path: FilePath): void {
    this.setProgramDirty();
    this.listener?.onSourceFileCreated(path.absolutePath);
  }

  onSourceFileInvalidated(path: FilePath): void {
    this.setProgramDirty();
    this.listener?.onSourceFileInvalidated(path.absolutePath);
  }

  onFileLoaded(path: FilePath): void {
    this.listener?.onFileContentLoaded(path.absolutePath);
  }

  loadProgramIfNeeded(): ts.Program {
    if (!this.currentProgram || this.programDirty) {
      this.updatingProgram = true;

      const oldProgram = this.currentProgram;

      this.listener?.onProgramWillChange();

      this.currentProgram = ts.createProgram(
        this.sourceFileManager.getScriptFileNames(),
        this.compilerOptions,
        new CompilerHost(this.fileManager, this.sourceFileManager),
        oldProgram,
      );

      this.updatingProgram = false;
      this.programDirty = false;

      this.listener?.onProgramChanged();
    }

    return this.currentProgram;
  }

  private setProgramDirty(): void {
    if (!this.updatingProgram) {
      this.programDirty = true;
    }
  }

  private resolveCompilerOptions(): ts.CompilerOptions {
    const tsConfigPath = this.fileManager.resolvePath('tsconfig.json');
    const tsConfigJsonFile = this.fileManager.getFile(tsConfigPath);
    const parseResult = ts.parseConfigFileTextToJson(tsConfigPath.absolutePath, tsConfigJsonFile.content);
    if (parseResult.error) {
      throw new Error(this.formatDiagnostic(parseResult.error));
    }

    const parsedCommandLine = ts.parseJsonConfigFileContent(
      parseResult.config!,
      new ParseConfigHost(this.fileManager),
      this.fileManager.getRootPath(),
      undefined,
      tsConfigPath.absolutePath,
    );

    if (parsedCommandLine.errors?.length) {
      // Ignore ts config errors for now
      // throw new Error(this.formatDiagnostics(parsedCommandLine.errors));
    }

    return parsedCommandLine.options;
  }
}
