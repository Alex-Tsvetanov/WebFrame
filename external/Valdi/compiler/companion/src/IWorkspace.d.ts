import {
  Diagnostic,
  DumpedInterface,
  DumpedSymbolsWithComments,
  DumpEnumResponseBody,
  DumpFunctionResponseBody,
} from './protocol';
import { AST, DumpedRootNode } from './AST';

export interface EmitResultEntry {
  fileName: string;
  content: string;
}

export interface EmitResult {
  entries: EmitResultEntry[];
  emitted: boolean;
}

export interface GetDiagnosticsResult {
  diagnostics: Diagnostic[];
  fileContent: string | undefined;
  hasError: boolean;
  timeTakenMs: number;
}

export interface DumpSymbolsWithCommentsResult {
  dumpedSymbols: DumpedRootNode[];
  references: AST.TypeReference[];
}

export interface OpenFileImportPath {
  relative: string;
  absolute: string;
}

export interface OpenFileResult {
  importPaths: OpenFileImportPath[];
  isNonModuleWithAmbiantDeclarations: boolean;
}

export interface IWorkspace {
  initialized: boolean;

  readonly workspaceRoot: string;

  initialize(): void;

  destroy(): void;

  registerInMemoryFile(fileName: string, fileContent: string): void;

  registerDiskFile(fileName: string, absoluteDiskPath: string): void;

  openFile(fileName: string): Promise<OpenFileResult>;

  emitFile(fileName: string): Promise<EmitResult>;

  getDiagnostics(fileName: string): Promise<GetDiagnosticsResult>;

  dumpSymbolsWithComments(fileName: string): Promise<DumpedSymbolsWithComments>;

  getInterfaceAST(fileName: string, position: number): Promise<DumpedInterface>;

  getEnumAST(fileName: string, position: number): Promise<DumpEnumResponseBody>;

  getFunctionAST(fileName: string, position: number): Promise<DumpFunctionResponseBody>;

  resolveImportPath(fromPath: string, toPath: string): string;

  getFileNameForImportPath(importPath: string): string;
}
