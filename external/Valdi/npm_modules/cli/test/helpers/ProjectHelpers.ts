import { spawn } from 'child_process';
import { mkdir, readFile, writeFile } from 'fs/promises';
import path from 'path';
import { eachLine } from './StreamHelpers';
import type { FilePosition, TSServerClient } from './TypeScriptClient';
import { lineAndOffset } from './TypeScriptClient';

type FileUpdater = (contents: Buffer) => Promise<Buffer>;

export interface ProjectConfig {
  rootPath: string;
  name: string;
  findFile: (fileName: string, withinModule?: string) => Promise<string>;
  updateFile: (fileName: string, withinModule: string | undefined, change: FileUpdater) => Promise<void>;
  createFile: (
    fileName: string,
    withinModule: string | undefined,
    source: Parameters<typeof writeFile>[1],
  ) => Promise<string>;
}

interface TaskContext {
  project: ProjectConfig;
  client: TSServerClient;
}

export function createProject(name: string, rootPath: string): ProjectConfig {
  const findFile: ProjectConfig['findFile'] = (fileName, withinModule) => {
    return new Promise((resolve, reject) => {
      const buffers: Buffer[] = [];

      const errorBuffers: Buffer[] = [];
      const directory = path.join(...['.', 'modules'].concat(withinModule ?? []));

      const child = spawn('find', [directory]);
      child.stdout.on('data', (chunk: Buffer) => {
        buffers.push(chunk);
      });

      child.stderr.on('data', (chunk: Buffer) => {
        errorBuffers.push(chunk);
      });

      child.on('close', code => {
        if (!!code && code > 0) {
          reject(new Error('exited with error code: ' + code.toString()));
        }

        const output = Buffer.concat(buffers).toString('utf8');

        const files = output.trim().split('\n');

        const found = files.find(file => file.endsWith(`/${fileName}`));

        if (!found) {
          reject(new Error('File not found: ' + fileName));
          return;
        }

        resolve(path.join(rootPath, found));
      });
    });
  };
  return {
    name,
    rootPath,
    findFile,
    updateFile: async (file, withinModule, updater) => {
      const path = await findFile(file, withinModule);
      const contents = await readFile(path);
      const updated = await updater(contents);
      await writeFile(path, updated);
    },
    createFile: async (filePath, withinModule, source) => {
      const fullPath = path.resolve(path.join(rootPath, withinModule ? `modules/${withinModule}` : '.', filePath));
      await mkdir(path.dirname(fullPath), { recursive: true });
      await writeFile(fullPath, source);
      return fullPath;
    },
  };
}

type LineMatcher = (line: string) => boolean;

export function lineMatches(search: string | RegExp): LineMatcher {
  return line => {
    if (typeof search === 'string') {
      return line.toString().includes(search);
    }
    return search.test(line);
  };
}

export const atBeginning: LineMatcher = () => true;

export function insertLine(position: 'before' | 'after', search: LineMatcher, content: string | Buffer): FileUpdater {
  const match = (content: Buffer | string): boolean => search(content.toString());
  return contents => {
    let found = false;
    let buffer = Buffer.from([]);
    for (const line of eachLine(contents)) {
      const bufferLine = typeof line === 'string' ? Buffer.from(line) : line;
      if (position === 'after') {
        buffer = Buffer.concat([buffer, bufferLine]);
      }
      if (!found && match(line)) {
        found = true;
        buffer = Buffer.concat([buffer, typeof content === 'string' ? Buffer.from(content) : content]);
      }
      if (position === 'before') {
        buffer = Buffer.concat([buffer, bufferLine]);
      }
    }
    return Promise.resolve(buffer);
  };
}

export function insertLineBefore(search: LineMatcher, content: string | Buffer): FileUpdater {
  return insertLine('before', search, content);
}

export function insertLineAfter(search: LineMatcher, content: string | Buffer): FileUpdater {
  return insertLine('after', search, content);
}

export async function findDefinitionForTokenInFile(
  context: TaskContext,
  token: string,
  file: string,
  valdiModuleName?: string,
): Promise<[pathDescriptor: string, position: FilePosition, location: { file: string }, token: string]> {
  const appSource = await context.project.findFile(file, valdiModuleName);
  const position = await lineAndOffset(token, appSource);
  const definition = await context.client.command('definition', {
    file: appSource,
    line: position.line,
    offset: position.offset + 1,
  });

  if ('success' in definition && !definition['success']) {
    console.error(definition['message']);
    throw new Error('failed to get typeDefinition');
  }

  const body = definition['body'] as { file: string; start: { line: number; offset: number } }[];
  const location = body.at(0);

  if (!location) {
    throw new Error('No library definitions found for: ' + token + ' at ' + JSON.stringify(position));
  }

  return [`${location.file}:${location.start.line}:${location.start.offset}`, position, location, token];
}
