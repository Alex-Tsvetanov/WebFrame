// copied from https://github.com/DefinitelyTyped/DefinitelyTyped/blob/master/types/istanbul-lib-coverage/index.d.ts

export interface CoverageSummaryData {
  lines: Totals;
  statements: Totals;
  branches: Totals;
  functions: Totals;
}

export interface CoverageMapData {
  [key: string]: FileCoverageData;
}

export interface Location {
  line: number;
  column: number;
}

export interface Range {
  start: Location;
  end: Location;
}

export interface BranchMapping {
  loc: Range;
  type: string;
  locations: Range[];
  line: number;
}

export interface FunctionMapping {
  name: string;
  decl: Range;
  loc: Range;
  line: number;
}

export interface FileCoverageData {
  path: string;
  statementMap: { [key: string]: Range };
  fnMap: { [key: string]: FunctionMapping };
  branchMap: { [key: string]: BranchMapping };
  s: { [key: string]: number };
  f: { [key: string]: number };
  b: { [key: string]: number[] };
}

export interface Totals {
  total: number;
  covered: number;
  skipped: number;
  pct: number;
}

export interface Coverage {
  covered: number;
  total: number;
  coverage: number;
}
