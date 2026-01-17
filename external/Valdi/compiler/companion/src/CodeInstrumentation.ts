import { createInstrumenter, Instrumenter, InstrumenterOptions } from 'istanbul-lib-instrument';
import { getRawSourceMap, SOURCE_MAP_PREFIX } from './SourceMapUtils';
import { RawSourceMap } from 'source-map';
import { fromObject } from 'convert-source-map';
import { AddCodeInstrumentationResponseBody } from './protocol';

export interface CodeInstrumentationFileConfig {
  sourceFilePath: string;
  fileContent: string;
}

export interface CodeInstrumentationResult {
  sourceFilePath: string;
  instrumentedFileContent: string | undefined;
  fileCoverage: string | undefined;
}

// CodeInstrumentation Class is responsible for adding the code instrumentation for the picked files from compiler
// Under that it uses istanbul-lib-instrument lib which adds the actual code instrumentation to the file
// design doc: https://docs.google.com/document/d/1YiwyxQfKgL_h5KGn4NPcePX8zCQeS1HXF14a-47Tk98
export class CodeInstrumentation {
  private static instance: CodeInstrumentation;
  private readonly instrumenter: Instrumenter;

  private constructor() {
    this.instrumenter = createInstrumenter({
      ignoreClassMethods: undefined,
      autoWrap: false,
      esModules: true,
      parserPlugins: [
        'asyncGenerators',
        'bigInt',
        'classProperties',
        'classPrivateProperties',
        'classPrivateMethods',
        'dynamicImport',
        'importMeta',
        'numericSeparator',
        'objectRestSpread',
        'optionalCatchBinding',
        'topLevelAwait',
        'typescript',
        'jsx',
      ],
      compact: true,
      preserveComments: true,
      produceSourceMap: true,
    } as Partial<InstrumenterOptions>);
  }

  public static getInstance(): CodeInstrumentation {
    if (!CodeInstrumentation.instance) {
      CodeInstrumentation.instance = new CodeInstrumentation();
    }

    return CodeInstrumentation.instance;
  }

  public instrumentFiles(files: [CodeInstrumentationFileConfig]): AddCodeInstrumentationResponseBody {
    const codeInstrumentationExecution = files.map((file) => this.instrumentFile(file));

    return { results: codeInstrumentationExecution };
  }

  public instrumentFile(file: CodeInstrumentationFileConfig): CodeInstrumentationResult {
    let data = file.fileContent;
    const sourceMap = getRawSourceMap(data);

    if (data.includes(SOURCE_MAP_PREFIX)) {
      data = data.split(SOURCE_MAP_PREFIX)[0];
    }

    let rawSourceMap: Partial<RawSourceMap> | undefined = undefined;

    if (sourceMap?.mappings) {
      rawSourceMap = {
        version: 3,
        sources: [file.sourceFilePath],
        mappings: sourceMap.mappings,
      };
    }

    let instrumented = this.instrumenter.instrumentSync(data, file.sourceFilePath, rawSourceMap as any);
    let lastSourceMap = this.instrumenter.lastSourceMap();
    let lastFileCoverage = this.instrumenter.lastFileCoverage();

    if (lastSourceMap) {
      instrumented += '\n' + fromObject(lastSourceMap).toComment();
    }

    let fileCoverage: string | undefined;
    if (lastFileCoverage) {
      fileCoverage = JSON.stringify(lastFileCoverage);
    }

    return { sourceFilePath: file.sourceFilePath, instrumentedFileContent: instrumented, fileCoverage };
  }
}
