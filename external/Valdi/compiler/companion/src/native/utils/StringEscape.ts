export interface CString {
  literalContent: string;
  length: number;
}

export function toCString(str: string): CString {
  const buffer = Buffer.from(str, 'utf-8');
  let literalContent = '"';

  for (let i = 0; i < buffer.length; i++) {
    const c = buffer[i];

    if (c >= 32 && c <= 126) {
      // Visible character
      if (c === 0x22) {
        literalContent += '\\"';
      } else if (c === 0x5c) {
        literalContent += '\\\\';
      } else {
        literalContent += String.fromCharCode(c);
      }
    } else if (c === 0x0a) {
      literalContent += '\\n';
    } else if (c === 0x09) {
      literalContent += '\\t';
    } else {
      literalContent += '\\x';
      literalContent += c.toString(16);
    }
  }

  literalContent += '"';

  return {
    literalContent,
    length: buffer.length,
  };
}

export function escapeCComment(str: string): string {
  return str.replace(/\*\//g, '*\\/').replace('/*', '/\\*');
}
