import { forEachFile } from '../utils/fileUtils';
import { ILogger } from '../logger/ILogger';
import { readFile, writeFile } from 'fs/promises';
import { CharCode, TextParser } from '../utils/TextParser';
import path = require('path');

interface ImportedSymbol {
  symbolName: string;
  symbolAlias?: string;
}

interface ImportStatement {
  filePositionStart: number;
  filePositionEnd: number;

  importPath: string | undefined;
  isTypeImport: boolean;
  namespaceSource?: string;
  symbolsList?: ImportedSymbol[];
  singleSymbol?: ImportedSymbol;
}

interface FileContentChunk {
  content: string;
  start: number;
  end: number;
  importStatement?: ImportStatement;
}

function resolveImportPath(projectDirectory: string, directory: string, importPath: string): string {
  if (importPath.startsWith('.')) {
    // Relative import
    return path.resolve(directory, importPath);
  } else {
    // Absolute import
    return path.join(projectDirectory, importPath);
  }
}

const enum VisitImportResult {
  noop,
  delete,
}

class ParsedFile {
  readonly directory: string;

  constructor(
    readonly filePath: string,
    readonly chunks: FileContentChunk[],
    readonly importStatements: ImportStatement[],
  ) {
    this.directory = path.dirname(filePath);
  }

  private visitImports(
    visitor: (importStatement: ImportStatement, importedSymbol: ImportedSymbol) => VisitImportResult,
  ): void {
    for (let i = 0; i < this.importStatements.length; i++) {
      const importStatement = this.importStatements[i];

      let removed = false;
      if (importStatement.singleSymbol) {
        const result = visitor(importStatement, importStatement.singleSymbol);
        if (result === VisitImportResult.delete) {
          importStatement.singleSymbol = undefined;
          removed = true;
        }
      }

      if (importStatement.symbolsList) {
        for (let j = 0; j < importStatement.symbolsList.length; j++) {
          const importedSymbol = importStatement.symbolsList[j];
          const result = visitor(importStatement, importedSymbol);
          if (result === VisitImportResult.delete) {
            importStatement.symbolsList.splice(j, 1);
            removed = true;
            break;
          }
        }
      }

      if (removed) {
        if (
          !importStatement.singleSymbol &&
          (!importStatement.symbolsList || importStatement.symbolsList.length === 0)
        ) {
          // Remove entire statement
          this.importStatements.slice(i, 1);
          const chunkIndex = this.chunks.findIndex((v) => v.importStatement === importStatement);
          this.chunks.splice(chunkIndex, 1);
          const newChunkAtIndex = this.chunks[chunkIndex];
          if (newChunkAtIndex && newChunkAtIndex.content[0] === '\n') {
            // Remove newline after removing our chunk if we have one
            newChunkAtIndex.content = newChunkAtIndex.content.substring(1);
          }
          i--;
        }
      }
    }
  }

  updateImportForSymbolName(
    importBaseDirectory: string,
    oldSymbolName: string,
    newSymbolName: string,
    oldImportPath: string,
    newImportPath: string,
  ): boolean {
    const oldAbsoluteImportPath = resolveImportPath(importBaseDirectory, importBaseDirectory, oldImportPath);
    let removed = false;
    this.visitImports((importStatement, importedSymbol): VisitImportResult => {
      if (!importStatement.importPath || importedSymbol.symbolName !== oldSymbolName) {
        return VisitImportResult.noop;
      }
      const absoluteImportPath = resolveImportPath(importBaseDirectory, this.directory, importStatement.importPath);
      if (absoluteImportPath !== oldAbsoluteImportPath) {
        return VisitImportResult.noop;
      }

      removed = true;
      return VisitImportResult.delete;
    });

    if (!removed) {
      return false;
    }

    const newAbsoluteImportPath = resolveImportPath(importBaseDirectory, importBaseDirectory, newImportPath);
    let updated = false;

    for (const importStatement of this.importStatements) {
      if (!importStatement.importPath) {
        continue;
      }

      const absoluteImportPath = resolveImportPath(importBaseDirectory, this.directory, importStatement.importPath);
      if (absoluteImportPath === newAbsoluteImportPath) {
        if (!importStatement.symbolsList) {
          importStatement.symbolsList = [];
        }

        importStatement.symbolsList.push({
          symbolName: newSymbolName,
        });
        updated = true;
        break;
      }
    }

    if (!updated) {
      // Add import statement
      this.insertImport(newSymbolName, newImportPath);
    }

    return true;
  }

  private insertImport(symbolName: string, importPath: string) {
    const newImportStatement: ImportStatement = {
      filePositionStart: 0,
      filePositionEnd: 0,
      importPath: importPath,
      isTypeImport: false,
      namespaceSource: undefined,
      symbolsList: [
        {
          symbolName: symbolName,
        },
      ],
      singleSymbol: undefined,
    };

    let insertionIndex: number;
    if (this.importStatements.length) {
      insertionIndex =
        this.chunks.findIndex((c) => c.importStatement === this.importStatements[this.importStatements.length - 1]) + 1;
    } else {
      insertionIndex = 0;
    }

    this.importStatements.push(newImportStatement);
    this.chunks.splice(insertionIndex, 0, { content: '', start: 0, end: 0, importStatement: newImportStatement });
    this.chunks.splice(insertionIndex, 0, { content: '\n', start: 0, end: 0, importStatement: undefined });
  }

  regenerateFileContent(): string {
    let out = '';

    for (const chunk of this.chunks) {
      if (chunk.importStatement) {
        out += this.generateImportStatement(chunk.importStatement);
      } else {
        out += chunk.content;
      }
    }

    return out;
  }

  private generateImportedSymbol(importedSymbol: ImportedSymbol): string {
    let out = importedSymbol.symbolName;
    if (importedSymbol.symbolAlias) {
      out += ' as ';
      out += importedSymbol.symbolAlias;
    }

    return out;
  }

  private generateImportedSymbolList(importedSymbolList: ImportedSymbol[]): string {
    const content = importedSymbolList.map((importedSymbol) => this.generateImportedSymbol(importedSymbol)).join(', ');
    return `{ ${content} }`;
  }

  private generateImportStatement(importStatement: ImportStatement): string {
    let out = 'import ';
    if (importStatement.isTypeImport) {
      out += 'type ';
    }

    if (!importStatement.singleSymbol && !importStatement.symbolsList) {
      if (importStatement.importPath) {
        out += `'${importStatement.importPath}'`;
      }
    } else {
      if (importStatement.singleSymbol) {
        out += this.generateImportedSymbol(importStatement.singleSymbol);

        if (importStatement.symbolsList) {
          out += ', ';
          out += this.generateImportedSymbolList(importStatement.symbolsList);
        }
      } else if (importStatement.symbolsList) {
        out += this.generateImportedSymbolList(importStatement.symbolsList);
      }

      if (importStatement.importPath) {
        out += ' from ';
        out += `'${importStatement.importPath}'`;
      } else if (importStatement.namespaceSource) {
        out += ' =';
        out += importStatement.namespaceSource;
      }
    }

    out += ';';
    return out;
  }
}

function makeImportedSymbol(symbolName: string, parser: TextParser): ImportedSymbol {
  if (parser.tryParse('as')) {
    return {
      symbolName,
      symbolAlias: parser.parseIdentifier(),
    };
  } else {
    return {
      symbolName,
    };
  }
}

function parseImportedSymbol(parser: TextParser): ImportedSymbol {
  const symbolName = parser.parseIdentifier();
  return makeImportedSymbol(symbolName, parser);
}

function parseImportedSymbolsList(parser: TextParser): ImportedSymbol[] {
  const symbolsList: ImportedSymbol[] = [];
  while (!parser.tryParse('}')) {
    symbolsList.push(parseImportedSymbol(parser));
    parser.tryParse(',');
  }

  return symbolsList;
}

function parseImport(parser: TextParser): ImportStatement {
  let symbolsList: ImportedSymbol[] | undefined;
  let singleSymbol: ImportedSymbol | undefined;

  const filePositionStart = parser.position;

  parser.parse('import ');

  // Ignore type keyword
  const isTypeImport = parser.tryParse('type ');

  if (parser.tryParse('*')) {
    singleSymbol = makeImportedSymbol('*', parser);
  } else if (parser.isAtIdentifier()) {
    singleSymbol = parseImportedSymbol(parser);

    if (parser.tryParse(',')) {
      parser.parse('{');
      symbolsList = parseImportedSymbolsList(parser);
    }
  } else if (parser.tryParse('{')) {
    symbolsList = parseImportedSymbolsList(parser);
  }

  if (symbolsList || singleSymbol) {
    if (parser.tryParse('=')) {
      const filePositionEnd = parser.position;
      const namespaceSource = parser.parseUntilCharCode(CharCode.semiColon);
      return {
        filePositionStart,
        filePositionEnd,
        isTypeImport,
        importPath: undefined,
        namespaceSource,
        symbolsList,
        singleSymbol,
      };
    } else {
      parser.parse('from');
    }
  }

  const importPath = parser.parseQuotedString();
  const filePositionEnd = parser.position;

  parser.tryParse(';');

  return {
    filePositionStart,
    filePositionEnd,
    isTypeImport,
    importPath,
    namespaceSource: undefined,
    symbolsList,
    singleSymbol,
  };
}

function appendChunkIfNeeded(parser: TextParser, output: FileContentChunk[]) {
  let lastEndPosition = 0;

  if (output.length > 0) {
    lastEndPosition = output[output.length - 1].end;
  }

  if (parser.position === lastEndPosition) {
    return;
  }

  output.push({
    start: lastEndPosition,
    end: parser.position,
    content: parser.text.substring(lastEndPosition, parser.position),
  });
}

async function parseFile(filePath: string): Promise<ParsedFile> {
  try {
    const fileContent = await readFile(filePath, { encoding: 'utf-8' });
    const parser = new TextParser(fileContent);

    const chunks: FileContentChunk[] = [];

    const imports: ImportStatement[] = [];

    while (!parser.isAtEnd()) {
      if (parser.peek('import ')) {
        appendChunkIfNeeded(parser, chunks);
        const importStatement = parseImport(parser);
        imports.push(importStatement);
        appendChunkIfNeeded(parser, chunks);
        chunks[chunks.length - 1].importStatement = importStatement;
      } else {
        parser.skipCurrentLine();
      }
    }

    appendChunkIfNeeded(parser, chunks);

    return new ParsedFile(filePath, chunks, imports);
  } catch (err: any) {
    err.message = `While parsing: ${filePath}: ${err.message}`;
    throw err;
  }
}

export async function rewriteImports(
  logger: ILogger | undefined,
  projectDirectory: string,
  oldTypeName: string,
  newTypeName: string,
  oldImportPath: string,
  newImportPath: string,
): Promise<void> {
  const allTypeScriptFiles: string[] = [];
  await forEachFile(projectDirectory, (filePath) => {
    if (filePath.endsWith('.ts') || filePath.endsWith('.tsx')) {
      allTypeScriptFiles.push(filePath);
    }
  });

  logger?.info?.(`Parsing ${allTypeScriptFiles.length} files...`);

  const parsedFiles = await Promise.all(allTypeScriptFiles.map((f) => parseFile(f)));

  logger?.info?.(
    `Rewriting type ${newTypeName} in import path '${newImportPath}' (from ${oldTypeName} with import path '${oldImportPath}'))`,
  );

  const pendingPromises: Promise<void>[] = [];

  for (const parsedFile of parsedFiles) {
    if (
      parsedFile.updateImportForSymbolName(projectDirectory, oldTypeName, newTypeName, oldImportPath, newImportPath)
    ) {
      logger?.info?.(`Found import in ${parsedFile.filePath}`);
      const updatedFileContent = parsedFile.regenerateFileContent();
      pendingPromises.push(writeFile(parsedFile.filePath, updatedFileContent, { encoding: 'utf-8' }));
    }
  }

  await Promise.all(pendingPromises);
}
