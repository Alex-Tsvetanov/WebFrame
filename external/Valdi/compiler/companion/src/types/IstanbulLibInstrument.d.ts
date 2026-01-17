// copied from https://github.com/DefinitelyTyped/DefinitelyTyped/blob/master/types/istanbul-lib-instrument/index.d.ts
// API docs for 4.0.3 version https://github.com/istanbuljs/istanbuljs/blob/istanbul-lib-instrument%404.0.3/packages/istanbul-lib-instrument/api.md

declare module 'istanbul-lib-instrument' {
  import { FileCoverage, FileCoverageData } from 'istanbul-lib-coverage';
  import { RawSourceMap } from 'source-map';
  import * as babelTypes from '@babel/types';
  export interface InstrumenterOptions {
    coverageVariable: string;
    preserveComments: boolean;
    compact: boolean;
    esModules: boolean;
    autoWrap: boolean;
    produceSourceMap: boolean;
    ignoreClassMethods: string[];
    sourceMapUrlCallback(filename: string, url: string): void;
    debug: boolean;
    parserPlugins: string[];
  }

  export type InstrumenterCallback = (error: Error | null, code: string) => void;

  export class Instrumenter {
    fileCoverage: FileCoverage;
    sourceMap: RawSourceMap | null;
    opts: InstrumenterOptions;

    constructor(options?: Partial<InstrumenterOptions>);

    normalizeOpts(options?: Partial<InstrumenterOptions>): InstrumenterOptions;

    instrumentSync(code: string, filename: string, inputSourceMap?: RawSourceMap): string;

    instrument(
      code: string,
      filenameOrCallback: string | InstrumenterCallback,
      callback?: InstrumenterCallback,
      inputSourceMap?: RawSourceMap,
    ): void;

    lastFileCoverage(): FileCoverageData;
    lastSourceMap(): RawSourceMap;
  }

  export function createInstrumenter(options?: Partial<InstrumenterOptions>): Instrumenter;

  export interface InitialCoverage {
    path: string;
    hash: string;
    gcv: any;
    coverageData: any;
  }

  export function readInitialCoverage(code: string): InitialCoverage;

  export interface Visitor {
    enter(path: string): void;
    exit(path: string): { fileCoverage: FileCoverage; sourceMappingURL: string };
  }

  export interface VisitorOptions {
    coverageVariable: string;
    inputSourceMap: RawSourceMap;
  }

  export function programVisitor(
    types: typeof babelTypes,
    sourceFilePath?: string,
    opts?: Partial<VisitorOptions>,
  ): Visitor;
}
