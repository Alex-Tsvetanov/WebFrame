import path = require('path');
import * as fs from 'fs';
import { VirtualFileSystem } from './VirtualFileSystem';
import { Lazy } from '../utils/Lazy';

export interface FilePath {
  absolutePath: string;
  pathComponents: readonly string[];
  pathExtension: string;
}

export interface File {
  readonly content: string;
}

export interface IFileManagerListener {
  onFileLoaded(path: FilePath): void;
}

export class FileManager {
  private virtualFileSystem = new VirtualFileSystem<() => File>();

  private rootPath: string;

  constructor(rootPath: string, readonly listener?: IFileManagerListener) {
    this.rootPath = rootPath;
  }

  clear(): void {
    this.virtualFileSystem.clear();
  }

  getCurrentDirectory(): string {
    return this.rootPath;
  }

  getRootPath(): string {
    return this.rootPath;
  }

  resolvePath(relativePath: string): FilePath {
    const absolutePath = path.resolve(this.rootPath, relativePath);
    let pathComponents: string[];
    if (absolutePath === path.sep) {
      // Special case for / : we don't use absolutePath.split() as it will
      // return two components
      pathComponents = [''];
    } else {
      pathComponents = absolutePath.split(path.sep);
    }

    const pathExtension = path.extname(relativePath);
    return {
      absolutePath,
      pathComponents,
      pathExtension,
    };
  }

  getDirectories(path: FilePath): FilePath[] {
    let entryNames = this.virtualFileSystem.getDirectoryEntryNames(path.pathComponents);
    if (!entryNames) {
      throw new Error(`Directory ${path.absolutePath} does not exist`);
    }

    return entryNames.map((e) => this.resolvePath(e));
  }

  fileExists(path: FilePath): boolean {
    return this.virtualFileSystem.fileExists(path.pathComponents);
  }

  directoryExists(path: FilePath): boolean {
    return this.virtualFileSystem.directoryExists(path.pathComponents);
  }

  addFileInMemory(path: FilePath, fileContent: string): void {
    this.virtualFileSystem.addFile(path.pathComponents, () => {
      return {
        content: fileContent,
      };
    });
  }

  addFileInDisk(path: FilePath, absolutePathOnDisk: string, cacheAfterReading: boolean = false): void {
    const reader: () => File = () => {
      const content = fs.readFileSync(absolutePathOnDisk, 'utf-8');
      return {
        content,
      };
    };

    if (cacheAfterReading) {
      const lazy = new Lazy(reader);
      this.virtualFileSystem.addFile(path.pathComponents, () => lazy.target);
    } else {
      this.virtualFileSystem.addFile(path.pathComponents, reader);
    }
  }

  getFile(path: FilePath): File {
    let fileProvider = this.virtualFileSystem.getFile(path.pathComponents);
    if (!fileProvider) {
      throw new Error(`File ${path.absolutePath} does not exist`);
    }

    const file = fileProvider();

    this.listener?.onFileLoaded(path);

    return file;
  }
}
