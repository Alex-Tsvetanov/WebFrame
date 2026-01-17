import { createHash } from 'crypto';

/**
 * Generates the SHA256 hash of a file.
 * @param fileContent - The content of the file represented as a UInt8Array.
 * @returns the SHA256 hash of the file.
 */
export function generateSHA256Hash(fileContent: Uint8Array): string {
  return createHash('sha256').update(fileContent).digest('hex');
}

/**
 * Generates the SHA256 hash of a string.
 * @param data - The content of the file represented as a string.
 * @returns the SHA256 hash of the file.
 */
export function generateSHA256HashFromString(data: string): string {
  return generateSHA256HashFromStrings([data]);
}

/**
 * Generates the SHA256 hash of a set of strings
 * @param data - The content of the file represented as an array of string
 * @returns the SHA256 hash of the file.
 */
export function generateSHA256HashFromStrings(data: string[]): string {
  const hash = createHash('sha256');
  for (const entry of data) {
    hash.update(entry, 'utf-8');
  }

  return hash.digest('hex');
}
