import * as SourceMapModule from 'source-map';
import * as path from 'path';

export const SOURCE_MAP_PREFIX = `//# sourceMappingURL=data:application/json;base64,`;

export function getRawSourceMap(fileContent: string): SourceMapModule.RawSourceMap | undefined {
  const index = fileContent.indexOf(SOURCE_MAP_PREFIX);
  if (index < 0) {
    return undefined;
  }

  const str = fileContent.substr(SOURCE_MAP_PREFIX.length + index);
  const sourceMapJson = Buffer.from(str, 'base64').toString('utf8');
  return JSON.parse(sourceMapJson) as SourceMapModule.RawSourceMap;
}

export function getSourceMap(fileContent: string): SourceMapModule.SourceMapConsumer | undefined {
  const rawSourceMap = getRawSourceMap(fileContent);
  if (!rawSourceMap) {
    return undefined;
  }
  return new SourceMapModule.SourceMapConsumer(rawSourceMap);
}

function insertGeneratedPosition(line: number, column: number, generatedKeys: { [key: string]: boolean }): boolean {
  const key = `${line}:${column}`;
  if (generatedKeys[key]) {
    return false;
  }
  generatedKeys[key] = true;
  return true;
}

export function mergeSourceMaps(originalFile: string, generatedFile: string): string {
  const rawOriginalSourceMap = getRawSourceMap(originalFile);
  if (!rawOriginalSourceMap) {
    return generatedFile;
  }

  const rawGeneratedSourceMap = getRawSourceMap(generatedFile);
  if (!rawGeneratedSourceMap) {
    return generatedFile;
  }

  if (rawOriginalSourceMap.sources.length !== 1 || rawGeneratedSourceMap.sources.length !== 1) {
    throw new Error(
      `Can only merge source maps from a single source. Got ${rawOriginalSourceMap.sources} and ${rawGeneratedSourceMap.sources}`,
    );
  }

  let originalFileName = rawOriginalSourceMap.sources[0];
  let intermediateFileName = rawGeneratedSourceMap.sources[0];
  const sourceToUse = originalFileName;

  if (rawOriginalSourceMap.sourceRoot) {
    originalFileName = path.resolve(rawOriginalSourceMap.sourceRoot, originalFileName);
  }

  if (rawGeneratedSourceMap.sourceRoot) {
    intermediateFileName = path.resolve(rawGeneratedSourceMap.sourceRoot, intermediateFileName);
  }

  const originalSourceMap = new SourceMapModule.SourceMapConsumer(rawOriginalSourceMap);
  const generatedSourceMap = new SourceMapModule.SourceMapConsumer(rawGeneratedSourceMap);

  let generator = new SourceMapModule.SourceMapGenerator({
    file: rawGeneratedSourceMap.file,
    sourceRoot: rawGeneratedSourceMap.sourceRoot,
  });

  let generatedKeys: { [key: string]: boolean } = {};

  // Add the new mappings, potentially updating the original position
  generatedSourceMap.eachMapping((mapping) => {
    const originalPosition = originalSourceMap.originalPositionFor({
      line: mapping.originalLine,
      column: mapping.originalColumn,
    });

    if (originalPosition.line) {
      if (!insertGeneratedPosition(mapping.generatedLine, mapping.generatedColumn, generatedKeys)) {
        return;
      }

      generator.addMapping({
        original: { line: originalPosition.line, column: originalPosition.column },
        generated: { line: mapping.generatedLine, column: mapping.generatedColumn },
        source: sourceToUse,
        name: mapping.name,
      });
    }
  });

  // Add the original mappings, potentially updating the generated position
  originalSourceMap.eachMapping((mapping) => {
    const generatedPosition = generatedSourceMap.generatedPositionFor({
      line: mapping.generatedLine,
      column: mapping.generatedColumn,
      source: intermediateFileName,
    });

    if (generatedPosition.line) {
      if (!insertGeneratedPosition(generatedPosition.line, generatedPosition.column, generatedKeys)) {
        return;
      }

      generator.addMapping({
        original: { line: mapping.originalLine, column: mapping.originalColumn },
        generated: {
          line: generatedPosition.line,
          column: generatedPosition.column,
        },
        source: sourceToUse,
        name: mapping.name,
      });
    }
  });

  if (rawOriginalSourceMap.sourcesContent && rawOriginalSourceMap.sourcesContent.length === 1) {
    generator.setSourceContent(sourceToUse, rawOriginalSourceMap.sourcesContent[0]);
  }

  const index = generatedFile.indexOf(SOURCE_MAP_PREFIX);

  let buffer = Buffer.from(generator.toString());
  let base64data = buffer.toString('base64');

  const filePrefix = generatedFile.substr(0, index);

  return `${filePrefix}${SOURCE_MAP_PREFIX}${base64data}`;
}
