import path from 'path';

/**
 * Version of path.relative() that resolves equal paths to .
 */
export function relativePathTo(from: string, to: string): string {
  const output = path.relative(from, to);
  if (!output) {
    return '.';
  }
  return output;
}
