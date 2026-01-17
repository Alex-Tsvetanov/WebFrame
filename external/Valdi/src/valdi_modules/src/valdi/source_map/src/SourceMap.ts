import { binarySearch } from 'coreutils/src/ArrayUtils';
import { Base64 } from 'coreutils/src/Base64';
import { arrayToString } from 'coreutils/src/Uint8ArrayUtils';
import { ISourceMap, MappedPosition } from './ISourceMap';
import { decode } from './VLQ';

export interface SourceMapJson {
  version?: number;
  file?: string;
  sourceRoot?: string;
  sources?: string[];
  names?: string[];
  mappings?: string;
  sourcesContent?: string[];
}

export interface MappedLineEntry {
  /**
   * The zero-based column number in the generated code that corresponds to this mapping segment.
   */
  generatedColumnNumber: number;

  /**
   * The zero-based column number of the next mapped line entry
   */
  nextGeneratedColumnNumber: number | undefined;

  /**
   * The zero-based line number in the source code that corresponds to this mapping segment.
   */
  originalLineNumber: number;

  /**
   * The zero-based line number in the source code that corresponds to this mapping segment.
   */
  originalColumnNumber: number;

  /**
   * The original name of the code referenced by this mapping entry
   */
  originalName: string | undefined;

  /**
   * The name of the file that originally contained this code
   */
  originalFileName: string | undefined;
}

export interface MappedLine {
  entries: MappedLineEntry[];
}

export class SourceMap implements ISourceMap {
  lineOffset: number;

  constructor(
    readonly file: string | undefined,
    readonly sourceRoot: string | undefined,
    readonly sources: string[] | undefined,
    readonly names: string[] | undefined,
    readonly lines: MappedLine[],
    readonly sourcesContent: string[] | undefined,
    lineOffset: number = 1,
  ) {
    this.lineOffset = lineOffset;
  }

  resolve(line: number, column: number | undefined, name: string | undefined): MappedPosition | undefined {
    const lineZeroIndexed = line - this.lineOffset;
    if (lineZeroIndexed >= this.lines.length || lineZeroIndexed < 0) {
      return undefined;
    }

    const mappedLine = this.lines[lineZeroIndexed];

    let entryIndex = -1;
    let columnOffset = 0;

    if (typeof column === 'number') {
      const columnZeroIndexed = column - 1;

      entryIndex = binarySearch(mappedLine.entries, item => {
        if (item.generatedColumnNumber > columnZeroIndexed) {
          return 1;
        } else if (item.generatedColumnNumber < columnZeroIndexed) {
          if (!item.nextGeneratedColumnNumber || item.nextGeneratedColumnNumber > columnZeroIndexed) {
            // We are within the range
            return 0;
          }

          return -1;
        }
        return 0;
      });

      if (entryIndex >= 0) {
        columnOffset = columnZeroIndexed - mappedLine.entries[entryIndex].generatedColumnNumber;
      }
    } else if (mappedLine.entries.length) {
      entryIndex = 0;
    }

    if (entryIndex < 0) {
      return undefined;
    }

    const entry = mappedLine.entries[entryIndex];

    return {
      fileName: entry.originalFileName,
      line: entry.originalLineNumber + 1,
      column: entry.originalColumnNumber + columnOffset + 1,
      name: entry.originalName ?? name,
    };
  }

  static parseFromBase64(base64String: string) {
    const byteArray = Base64.toByteArray(base64String);
    const jsonString = arrayToString(byteArray);
    return SourceMap.parse(jsonString);
  }

  static parse(sourceMapJson: string): SourceMap {
    const object = JSON.parse(sourceMapJson) as SourceMapJson;
    if (object.version !== 3) {
      throw new Error('SourceMap version must be 3');
    }

    if (!object.mappings) {
      throw new Error('Missing mappings');
    }

    let sources = object.sources ?? [];
    const names = object.names ?? [];

    sources = sources.map(source => {
      // Remove leading '/'
      if (source.charAt(0) === '/') {
        source = source.substr(1);
      }

      return source;
    });

    /**
      Logic ported from https://github.com/microsoft/sourcemap-toolkit/blob/master/src/SourcemapToolkit.SourcemapParser/MappingListParser.cs
       */

    const lines: MappedLine[] = [];
    const allMappings = object.mappings.split(';');

    let generatedLineNumber = 0;
    let generatedColumnNumber = 0;
    let sourcesListIndex = 0;
    let originalLineNumber = 0;
    let originalColumnNumber = 0;
    let namesListIndex = 0;

    for (const mappings of allMappings) {
      const segments = mappings.split(',');
      const entries: MappedLineEntry[] = [];

      for (const segment of segments) {
        if (!segment) {
          continue;
        }

        const fields = decode(segment);

        if (fields.length !== 1 && fields.length !== 4 && fields.length !== 5) {
          throw new Error(`Invalid mappings (found ${fields.length} fields in ${segment})`);
        }

        generatedColumnNumber += fields[0];

        let originalFileName: string | undefined;

        if (fields.length > 1) {
          sourcesListIndex += fields[1];
          originalLineNumber += fields[2];
          originalColumnNumber += fields[3];

          if (sourcesListIndex >= sources.length) {
            throw new Error(`Out of bounds sources index ${sourcesListIndex}`);
          }

          originalFileName = sources[sourcesListIndex];
        }

        let originalName: string | undefined;

        if (fields.length === 5) {
          namesListIndex += fields[4];

          if (namesListIndex >= names.length) {
            throw new Error(`Out of bounds names index ${namesListIndex}`);
          }
          originalName = names[namesListIndex];
        }

        if (entries.length) {
          entries[entries.length - 1].nextGeneratedColumnNumber = generatedColumnNumber;
        }

        entries.push({
          generatedColumnNumber,
          nextGeneratedColumnNumber: undefined,
          originalColumnNumber,
          originalLineNumber,
          originalFileName,
          originalName,
        });
      }

      lines.push({ entries });

      generatedLineNumber++;
      generatedColumnNumber = 0;
    }

    return new SourceMap(object.file, object.sourceRoot, object.sources, object.names, lines, object.sourcesContent);
  }
}

export class LineOffsetSourceMap implements ISourceMap {
  private readonly lineOffset: number;
  constructor(lineOffset: number) {
    this.lineOffset = lineOffset;
  }
  resolve(line: number, column: number | undefined, name: string | undefined): MappedPosition | undefined {
    const adjustedLine = line - this.lineOffset;
    if (adjustedLine < 0) {
      return undefined; // If the adjusted line is negative, return undefined
    }
    return {
      fileName: undefined,
      line: adjustedLine,
      column: column ?? 0,
      name: undefined,
    };
  }
}
