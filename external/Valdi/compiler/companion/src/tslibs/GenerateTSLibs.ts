import { forEachFile } from '../utils/fileUtils';
import path = require('path');
import fs = require('fs');
import { DB_FILENAME, TSLibEntry } from './TSLibsDatabase';

const INPUT_DIRECTORY = path.resolve(__dirname, '../../node_modules/typescript/lib');
const OUTPUT_DIRECTORY = path.resolve(__dirname);
const OUTPUT_FILE = path.join(OUTPUT_DIRECTORY, DB_FILENAME);

async function generateTSLibEntry(inputDirectory: string, filePath: string): Promise<TSLibEntry> {
  const fileContent = await fs.promises.readFile(filePath, 'utf-8');
  return {
    fileName: path.relative(inputDirectory, filePath),
    content: fileContent,
  };
}

export async function generateTSLibsFile(): Promise<void> {
  const entryPromises: Promise<TSLibEntry>[] = [];
  await forEachFile(INPUT_DIRECTORY, (filePath) => {
    const fileName = path.basename(filePath);
    if (fileName.startsWith('lib.') && fileName.endsWith('.d.ts')) {
      entryPromises.push(generateTSLibEntry(INPUT_DIRECTORY, filePath));
    }
  });

  const entries = await Promise.all(entryPromises);
  entries.sort((l, r) => l.fileName.localeCompare(r.fileName));

  const json = JSON.stringify(entries, null, 2);

  if (fs.existsSync(OUTPUT_FILE)) {
    await fs.promises.rm(OUTPUT_FILE);
  }

  await fs.promises.writeFile(OUTPUT_FILE, json);
}

generateTSLibsFile();
