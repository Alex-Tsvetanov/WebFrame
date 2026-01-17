import { RuntimeBase } from './RuntimeBase';

declare const runtime: RuntimeBase;

export function arrayToString(buf: Uint8Array): string {
  if (runtime.bytesToString) {
    return runtime.bytesToString(buf);
  }

  return String.fromCharCode.apply(null, buf as any);
}

export function stringToArray(str: string): Uint8Array {
  const buf = new ArrayBuffer(str.length);
  const bufView = new Uint8Array(buf);
  for (let i = 0, strLen = str.length; i < strLen; i++) {
    bufView[i] = str.charCodeAt(i);
  }
  return bufView;
}
