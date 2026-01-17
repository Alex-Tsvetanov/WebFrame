import { decodeIntoString, encodeString, Encoding } from './UnicodeNative';

export type TextEncoding = 'utf8' | 'utf16' | 'utf32' | 'utf-8' | 'utf-16' | 'utf-32';

function resolveEncoding(textEncoding: TextEncoding): Encoding {
  if (textEncoding === 'utf-8' || textEncoding == 'utf8') {
    return Encoding.utf8;
  } else if (textEncoding === 'utf-16' || textEncoding == 'utf16') {
    return Encoding.utf16;
  } else if (textEncoding === 'utf-32' || textEncoding == 'utf32') {
    return Encoding.utf32;
  }
  throw new Error(`Unsupported text encoding '${textEncoding}'`);
}

function encodingToString(encoding: Encoding): string {
  switch (encoding) {
    case Encoding.utf8:
      return 'utf-8';
    case Encoding.utf16:
      return 'utf-16';
    case Encoding.utf32:
      return 'utf-32';
  }
}

export class TextDecoder {
  get encoding(): string {
    return encodingToString(this._encoding);
  }

  get fatal(): boolean {
    return false;
  }

  get ignoreBOM(): boolean {
    return false;
  }

  private readonly _encoding: Encoding;

  constructor(encoding?: TextEncoding) {
    this._encoding = encoding ? resolveEncoding(encoding) : Encoding.utf8;
  }

  decode(buffer: ArrayBufferLike): string {
    return decodeIntoString(buffer, this._encoding);
  }
}

export class TextEncoder {
  get encoding(): string {
    return encodingToString(this._encoding);
  }

  private readonly _encoding: Encoding;

  constructor(encoding?: TextEncoding) {
    this._encoding = encoding ? resolveEncoding(encoding) : Encoding.utf8;
  }

  encode(str: string): Uint8Array {
    return new Uint8Array(encodeString(str, this._encoding));
  }

  encodeInto(str: string, buffer: ArrayBufferLike): TextEncoderEncodeIntoResult {
    throw new Error('encodeInto() is not yet implemented in Valdi');
  }
}
