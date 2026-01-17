import path from 'node:path';

import extract from 'extract-zip';

export async function decompressTo(inputFilePath: string, outputFilePath: string): Promise<void> {
  const archive = path.resolve(inputFilePath);
  const destination = path.resolve(outputFilePath);

  try {
    await extract(archive, { dir: destination });
  } catch (error: unknown) {
    throw new Error(`Failed to extract “${archive}” → “${destination}”: ${(error as Error).message}`);
  }
}
