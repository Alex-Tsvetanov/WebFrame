export interface MappedPosition {
  fileName: string | undefined;
  name: string | undefined;
  line: number;
  column: number;
}

export interface ISourceMap {
  resolve(line: number, column: number | undefined, name: string | undefined): MappedPosition | undefined;
}
