import { ILogger } from '../logger/ILogger';
import { CompileNativeRequestBody, CompileNativeResponseBody, EmittedFile } from '../protocol';
import { Workspace } from '../Workspace';
import * as path from 'path';
import { NativeCompiler, compileIRsToC } from './NativeCompiler';
import { Diagnostic } from '../protocol';
import * as ts from 'typescript';
import { writeFile } from 'fs/promises';
import * as fs from 'fs';
import { NativeCompilerIR } from './builder/NativeCompilerBuilderIR';
import { NativeCompilerOptions } from './NativeCompilerOptions';

function checkErrors(sourceFile: ts.SourceFile, diagnostics: Diagnostic[]) {
  for (const diagnostic of diagnostics) {
    if (diagnostic.category === 'error') {
      const diagnosticStart = diagnostic.start!;
      const errorPosition = sourceFile.getPositionOfLineAndCharacter(diagnosticStart.line - 1, diagnosticStart.offset);
      const fullText = sourceFile.getFullText();
      const sourceError = fullText.substring(errorPosition, Math.min(errorPosition + 50, fullText.length)).trim();
      throw Error(
        `TypeScript Error: ${diagnostic.text.trim()}\nAt line ${diagnostic.start?.line} and column ${
          diagnostic.start?.offset
        }:\n${sourceError}`,
      );
    }
  }
}

class CancelationTokenImpl implements ts.CancellationToken {
  private cancelled = false;

  requestCancellation(): void {
    this.cancelled = true;
  }

  isCancellationRequested(): boolean {
    return this.cancelled;
  }

  throwIfCancellationRequested(): void {
    if (this.isCancellationRequested()) {
      throw new ts.OperationCanceledException();
    }
  }
}

export function compileFile(
  workspace: Workspace,
  syntaxCheck: boolean,
  options: NativeCompilerOptions,
  inputFile: string,
  modulePath: string,
): NativeCompilerIR.Base[] {
  const openedFile = workspace.getOpenedFile(inputFile);
  const sourceFile = openedFile.sourceFile;
  const typeChecker = openedFile.workspaceProject.typeChecker;

  if (syntaxCheck) {
    const diagnostics = workspace.getDiagnosticsSync(inputFile);
    checkErrors(sourceFile, diagnostics.diagnostics);
  }

  if (Workspace.requiresTSXProcessor(inputFile)) {
    // If we need the TSX processor, we need to go through the regular TS compiler
    // to transpile the TSX AST before we compile the output using the NativeCompiler

    const cancelToken = new CancelationTokenImpl();

    const outputIR: NativeCompilerIR.Base[] = [];
    const transforms: ts.TransformerFactory<ts.SourceFile>[] = [
      (context) => (node) => {
        const compiler = new NativeCompiler(node, inputFile, modulePath, typeChecker, options);
        outputIR.push(...compiler.compile());

        // Once we are done compiled with the NativeCompiler, we can ask the TS compiler to stop
        // processing this file. We don't need TS to JS compilation.
        cancelToken.requestCancellation();

        // Return empty source file as we finished the processing
        return context.factory.updateSourceFile(
          node,
          [],
          node.isDeclarationFile,
          undefined,
          undefined,
          undefined,
          undefined,
        );
        return node;
      },
    ];

    workspace.doEmitFile(inputFile, cancelToken, {
      before: transforms,
    });

    return outputIR;
  } else {
    const compiler = new NativeCompiler(sourceFile, inputFile, modulePath, typeChecker, options);
    return compiler.compile();
  }
}

function generateNativeModuleName(relativePaths: string[]): string {
  if (relativePaths.length) {
    const possibleModuleName = relativePaths[0].split('/')[0];

    if (relativePaths.filter((v) => !v.startsWith(possibleModuleName)).length === 0) {
      // All modules start with the same folder, we use that as the name
      return `${possibleModuleName}_ts_native.c`;
    }
  }

  return 'module_ts_native.c';
}

export async function compileNative(
  workspace: Workspace,
  logger: ILogger | undefined,
  request: CompileNativeRequestBody,
): Promise<CompileNativeResponseBody> {
  const filesToOpen: string[] = [];

  for (const inputFile of request.inputFiles) {
    let relativePath: string;
    if (request.registerInputFiles) {
      const stripIndex = inputFile.indexOf(request.stripIncludePrefix);
      if (stripIndex < 0) {
        throw new Error(`Invalid strip include prefix '${request.stripIncludePrefix}' for file path ${inputFile}`);
      }
      relativePath = inputFile.substring(stripIndex + request.stripIncludePrefix.length + 1);

      workspace.registerDiskFile(relativePath, inputFile);
    } else {
      relativePath = inputFile;
    }

    if (
      (inputFile.endsWith('.ts') && !inputFile.endsWith('.d.ts')) ||
      inputFile.endsWith('.tsx') ||
      inputFile.endsWith('.js')
    ) {
      filesToOpen.push(relativePath);
    }
  }

  if (!workspace.initialized) {
    await workspace.initialize();
  }

  const openedFiles = await Promise.all(filesToOpen.map((file) => workspace.openFile(file)));
  const allIRs: NativeCompilerIR.Base[] = [];
  const shouldCheckSyntax = !!request.registerInputFiles;

  const options = {
    optimizeSlots: true,
    optimizeVarRefs: true,

    // smaller code size and faster
    optimizeNullChecks: true,
    mergeSetProperties: true,
    foldConstants: true,
    optimizeVarRefLoads: true,
    optimizeAssignments: true,
    inlinePropertyCache: true,
    enableIntrinsics: true, // unsafe, replace some builtin functions with intrinsics

    mergeReleases: false, // smaller code size, very slightly slower
    autoRelease: false, // smaller code size, slightly slower
    noinlineRetainRelease: false, // smaller code size, slower
  };
  let fileIndex = 0;
  for (const openedFile of openedFiles) {
    const relativePath = filesToOpen[fileIndex];

    // Remove file extension for the module loading path
    const parsedRelativePath = path.parse(relativePath);

    const moduleLoadingPath = path.format({
      root: parsedRelativePath.root,
      dir: parsedRelativePath.dir,
      base: undefined,
      ext: undefined,
      name: parsedRelativePath.name,
    });

    try {
      const result = compileFile(workspace, shouldCheckSyntax, options, relativePath, moduleLoadingPath);
      logger?.debug?.(`File ${relativePath} compiled succesfully`);

      allIRs.push(...result);
    } catch (error: any) {
      logger?.error?.(`File ${moduleLoadingPath} failed to compile: ${error.message}\nStack: ${error.stack}`);
      throw error;
    }

    fileIndex++;
  }

  const generatedCCode = compileIRsToC(allIRs, options);
  let emittedFileContent: string | undefined;

  if (request.outputFile) {
    const outputDirectoryPath = path.dirname(request.outputFile);
    if (!fs.existsSync(outputDirectoryPath)) {
      fs.mkdirSync(outputDirectoryPath, { recursive: true });
    }
    await writeFile(request.outputFile, generatedCCode.source);
  } else {
    emittedFileContent = generatedCCode.source;
  }

  const emittedFile: EmittedFile = {
    fileName: generateNativeModuleName(filesToOpen),
    content: emittedFileContent,
  };

  return { files: [emittedFile] };
}
