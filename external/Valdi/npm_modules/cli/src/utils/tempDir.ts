import crypto from 'crypto';
import * as fs from 'node:fs';
import os from 'os';
import path from 'path';

export function makeTempDir(): string {
  return path.join(os.tmpdir(), crypto.randomUUID());
}

export async function withTempDir<T>(receiveTempDir: (tempDir: string) => Promise<T>): Promise<T> {
  const tempDir = makeTempDir();

  fs.mkdirSync(tempDir);

  try {
    return await receiveTempDir(tempDir);
  } finally {
    try {
      fs.rmdirSync(tempDir);
    } catch {
      // no-op
    }
  }
}
