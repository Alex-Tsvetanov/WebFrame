export const enum Encoding {
  utf8 = 1,
  utf16 = 2,
  utf32 = 3,
}

export interface FromCodepointsOutput {
  str: string;
  buffer: ArrayBuffer;
}

export function strToCodepoints(input: string, normalize: boolean, disableCategorization: boolean): ArrayBuffer;

export function codepointsToStr(
  codepoints: number[],
  normalize: boolean,
  disableCategorization: boolean,
): FromCodepointsOutput;

export function encodeString(str: string, encoding: Encoding): ArrayBuffer;

export function decodeIntoString(buffer: ArrayBufferLike, encoding: Encoding): string;
