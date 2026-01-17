import path = require('path');

class VirtualFileSystemEntry<T> {
  readonly parent: VirtualFileSystemEntry<T> | undefined;
  readonly name: string;

  private directory: Map<string, VirtualFileSystemEntry<T>> | undefined;
  private file: T | undefined;

  constructor(parent: VirtualFileSystemEntry<T> | undefined, name: string) {
    this.parent = parent;
    this.name = name;
    this.directory = parent ? undefined : new Map();
    this.file = undefined;
  }

  clear(): void {
    if (this.directory) {
      this.directory.clear();
      this.directory = undefined;
    }
    this.file = undefined;
  }

  getAbsoluteName(): string {
    let components: string[] = [];

    let current: VirtualFileSystemEntry<T> | undefined = this;
    while (current) {
      components.push(current.name);
      current = current.parent;
    }

    return components.reverse().join(path.sep);
  }

  emplaceNested(name: string): VirtualFileSystemEntry<T> {
    const directory = this.setAsDirectory();

    let nestedEntry = directory.get(name);
    if (!nestedEntry) {
      nestedEntry = new VirtualFileSystemEntry(this, name);
      directory.set(name, nestedEntry);
    }

    return nestedEntry;
  }

  private setAsDirectory(): Map<string, VirtualFileSystemEntry<T>> {
    if (!this.directory) {
      if (this.file) {
        throw new Error(`Cannot create directory "${this.name}": "${this.getAbsoluteName()}" is a file`);
      }

      this.directory = new Map();
    }

    return this.directory;
  }

  getNested(name: string): VirtualFileSystemEntry<T> | undefined {
    return this.directory?.get(name);
  }

  remove(): void {
    if (!this.parent) {
      throw new Error('Cannot remove root entry');
    }

    this.parent.directory!.delete(this.name);
  }

  setFile(file: T): void {
    if (this.directory) {
      throw new Error(`Cannot create file "${this.name}": "${this.getAbsoluteName()}" is a directory`);
    }

    this.file = file;
  }

  setDirectory(): boolean {
    if (this.directory) {
      return false;
    }
    this.setAsDirectory();
    return true;
  }

  getFile(): T | undefined {
    return this.file;
  }

  isFile(): boolean {
    return !!this.file;
  }

  isDirectory(): boolean {
    return !!this.directory;
  }

  getEntryNames(): string[] | undefined {
    const keys = this.directory?.keys();
    if (!keys) {
      return undefined;
    }

    return [...keys];
  }
}

export class VirtualFileSystem<T> {
  private root = new VirtualFileSystemEntry<T>(undefined, '');

  clear(): void {
    this.root.clear();
  }

  addFile(pathComponents: readonly string[], file: T) {
    const entry = this.emplaceEntry(pathComponents);

    entry.setFile(file);
  }

  addDirectory(pathComponents: readonly string[]): boolean {
    const entry = this.emplaceEntry(pathComponents);

    return entry.setDirectory();
  }

  getFile(pathComponent: readonly string[]): T | undefined {
    return this.getEntry(pathComponent)?.getFile();
  }

  remove(pathComponents: readonly string[]): boolean {
    const entry = this.getEntry(pathComponents);

    if (!entry) {
      return false;
    }

    entry.remove();
    return true;
  }

  fileExists(pathComponents: readonly string[]): boolean {
    return this.getEntry(pathComponents)?.isFile() ?? false;
  }

  directoryExists(pathComponents: readonly string[]): boolean {
    return this.getEntry(pathComponents)?.isDirectory() ?? false;
  }

  getDirectoryEntryNames(pathComponents: readonly string[]): string[] | undefined {
    return this.getEntry(pathComponents)?.getEntryNames();
  }

  private getEntry(pathComponents: readonly string[]): VirtualFileSystemEntry<T> | undefined {
    let current = this.root;

    for (const pathComponent of pathComponents) {
      const newCurrent = current.getNested(pathComponent);
      if (!newCurrent) {
        return undefined;
      }

      current = newCurrent;
    }

    return current;
  }

  private emplaceEntry(pathComponents: readonly string[]): VirtualFileSystemEntry<T> {
    let current = this.root;

    for (const pathComponent of pathComponents) {
      current = current.emplaceNested(pathComponent);
    }

    return current;
  }
}
