import * as ts from 'typescript';

// Taken from ts-morph
// These are hacks are re-use some private utilities
// from typescript

export function matchFiles(
  this: any,
  path: string,
  extensions: ReadonlyArray<string>,
  excludes: ReadonlyArray<string>,
  includes: ReadonlyArray<string>,
  useCaseSensitiveFileNames: boolean,
  currentDirectory: string,
  depth: number | undefined,
  getEntries: (path: string) => FileSystemEntries,
  realpath: (path: string) => string,
  directoryExists: (path: string) => boolean,
): string[] {
  return (ts as any).matchFiles.apply(this, arguments);
}

export interface FileSystemEntries {
  readonly files: ReadonlyArray<string>;
  readonly directories: ReadonlyArray<string>;
}
