import * as ts from 'typescript';
import { FileManager, FilePath } from './FileManager';

class SourceFileEntry {
  private scriptSnapshot: ts.IScriptSnapshot | undefined = undefined;
  private sourceFile: ts.SourceFile | undefined = undefined;
  private version = 0;

  constructor(readonly path: FilePath) {}

  getVersion(): string {
    return this.version.toString();
  }

  getScriptKind(): ts.ScriptKind {
    switch (this.path.pathExtension) {
      case 'js':
        return ts.ScriptKind.JS;
      case 'json':
        return ts.ScriptKind.JSON;
      case 'ts':
        return ts.ScriptKind.TS;
      case 'tsx':
        return ts.ScriptKind.TSX;
      default:
        return ts.ScriptKind.Unknown;
    }
  }

  getSnapshot(fileManager: FileManager): ts.IScriptSnapshot {
    if (!this.scriptSnapshot) {
      const file = fileManager.getFile(this.path);
      this.scriptSnapshot = ts.ScriptSnapshot.fromString(file.content);
    }

    return this.scriptSnapshot;
  }

  getSourceFile(): ts.SourceFile | undefined {
    return this.sourceFile;
  }

  createSourceFile(
    fileManager: FileManager,
    languageVersionOrOptions: ts.ScriptTarget | ts.CreateSourceFileOptions,
  ): ts.SourceFile {
    this.sourceFile = ts.createLanguageServiceSourceFile(
      this.path.absolutePath,
      this.getSnapshot(fileManager),
      languageVersionOrOptions,
      this.getVersion(),
      true,
      this.getScriptKind(),
    );

    return this.sourceFile;
  }

  invalidate(): boolean {
    if (this.scriptSnapshot || this.sourceFile) {
      this.version++;
      this.scriptSnapshot = undefined;
      this.sourceFile = undefined;

      return true;
    } else {
      return false;
    }
  }
}

export interface ISourceFileManagerListener {
  onSourceFileInvalidated(path: FilePath): void;
  onSourceFileCreated(path: FilePath): void;
}

export class SourceFileManager {
  private entries = new Map<string, SourceFileEntry>();
  private version = 0;

  constructor(readonly fileManager: FileManager, readonly listener?: ISourceFileManagerListener) {}

  clear(): void {
    this.entries.clear();
  }

  getVersion(): number {
    return this.version++;
  }

  getScriptFileNames(): string[] {
    const output: string[] = [];

    for (const [key, value] of this.entries) {
      if (value.getSourceFile()) {
        output.push(key);
      }
    }

    return output.sort();
  }

  getScriptVersion(filePath: string): string {
    const resolvedFilePath = this.fileManager.resolvePath(filePath);
    return this.getOrCreateEntry(resolvedFilePath).getVersion();
  }

  getScriptSnapshot(filePath: string) {
    const resolvedFilePath = this.fileManager.resolvePath(filePath);
    return this.getOrCreateEntry(resolvedFilePath).getSnapshot(this.fileManager);
  }

  getScriptKind(filePath: string): ts.ScriptKind {
    const resolvedFilePath = this.fileManager.resolvePath(filePath);
    return this.getOrCreateEntry(resolvedFilePath).getScriptKind();
  }

  getOrCreateSourceFile(
    filePath: string,
    languageVersionOrOptions: ts.ScriptTarget | ts.CreateSourceFileOptions,
  ): ts.SourceFile {
    const resolvedFilePath = this.fileManager.resolvePath(filePath);
    const entry = this.getOrCreateEntry(resolvedFilePath);

    let sourceFile = entry.getSourceFile();
    if (!sourceFile) {
      sourceFile = entry.createSourceFile(this.fileManager, languageVersionOrOptions);

      this.listener?.onSourceFileCreated(resolvedFilePath);
    }

    return sourceFile;
  }

  getSourceFile(filePath: string): ts.SourceFile | undefined {
    const resolvedFilePath = this.fileManager.resolvePath(filePath);
    const entry = this.getOrCreateEntry(resolvedFilePath);
    return entry.getSourceFile();
  }

  removeSourceFile(filePath: string) {
    const resolvedFilePath = this.fileManager.resolvePath(filePath);

    this.invalidateSourceFile(resolvedFilePath);
  }

  invalidateSourceFile(filePath: FilePath) {
    const entry = this.entries.get(filePath.absolutePath);
    if (!entry) {
      return;
    }

    if (entry.invalidate()) {
      this.listener?.onSourceFileInvalidated(filePath);
    }
  }

  private getOrCreateEntry(filePath: FilePath): SourceFileEntry {
    let entry = this.entries.get(filePath.absolutePath);
    if (!entry) {
      entry = new SourceFileEntry(filePath);
    }
    this.entries.set(filePath.absolutePath, entry);

    return entry;
  }
}
