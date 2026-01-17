// Code in this file is adapted from uuid-tool:
// https://github.com/domske/uuid-tool/blob/master/src/uuid.ts

export function uuidStringToBytes(uuid: string): Uint8Array {
  const bytes = (uuid.trim().replace(/-/g, '').match(/.{2}/g) || []).map(b => parseInt(b, 16));
  return Uint8Array.of(...bytes);
}

export function uuidStringToBytesList(uuids: string[]): Uint8Array[] {
  return uuids.map(uuidStringToBytes);
}

export function uuidBytesToString(uuid: Uint8Array): string {
  return Array.from(uuid)
    .map(b => ('00' + b.toString(16)).slice(-2))
    .join('')
    .replace(/(.{8})(.{4})(.{4})(.{4})(.{12})/, '$1-$2-$3-$4-$5');
}

export function uuidBytesToStringList(uuids: readonly Uint8Array[]): string[] {
  return uuids.map(uuidBytesToString);
}
