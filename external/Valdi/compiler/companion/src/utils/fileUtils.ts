import { stat, readdir } from 'fs/promises';
import * as path from 'path';

// Asynchronous utility function to check if a file/directory exists at the provided path.
export async function fileExists(path: string): Promise<boolean> {
  try {
    await stat(path);
  } catch (error) {
    const nodeError = error as NodeJS.ErrnoException;
    if (nodeError.code && nodeError.code === 'ENOENT') {
      // path doesn't exist
      return false;
    }

    // some other error - rethrow
    throw error;
  }
  return true;
}

export async function forEachFile(dir: string, cb: (filePath: string) => void): Promise<void> {
  const dirents = await readdir(dir, { withFileTypes: true });

  for (const dirent of dirents) {
    const filePath = path.join(dir, dirent.name);
    if (dirent.isDirectory()) {
      await forEachFile(filePath, cb);
    } else {
      cb(filePath);
    }
  }
}
