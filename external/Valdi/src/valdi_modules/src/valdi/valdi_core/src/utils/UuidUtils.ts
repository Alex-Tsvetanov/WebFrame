export function uuidBytesToString(bytes: Uint8Array): string {
  let out = '';
  for (let i = 0; i < 16; i++) {
    if (i === 4 || i === 6 || i === 8 || i === 10) {
      out += '-';
    }
    out += byteToString(bytes[i]);
  }
  return out;
}

function byteToString(byte: number): string {
  const out = byte.toString(16);
  if (out.length < 2) {
    return '0' + out;
  }
  return out;
}

export function uuidToString(lowBits: Long, highBits: Long): string {
  const bytes = new Uint8Array([...highBits.toBytes(), ...lowBits.toBytes()]);
  return uuidBytesToString(bytes);
}

/**
 * Converts a string to a UUID object, containg low and high bits
 * @param str - The string to convert
 */
export function stringToUUID(str: string): { lowBits: Long; highBits: Long } {
  const UUID_SIZE = 16;
  const lowBytes = [UUID_SIZE / 2];
  const highBytes = [UUID_SIZE / 2];

  let strIndex = 0;
  for (let i = 0; i < UUID_SIZE / 2; i++) {
    const byteHex = str.slice(strIndex, strIndex + 2);
    highBytes[i] = Number.parseInt(byteHex, 16);

    strIndex += 2;
    if (str[strIndex] === '-') {
      strIndex++;
    }
  }
  for (let i = 0; i < UUID_SIZE / 2; i++) {
    const byteHex = str.slice(strIndex, strIndex + 2);
    lowBytes[i] = Number.parseInt(byteHex, 16);

    strIndex += 2;
    if (str[strIndex] === '-') {
      strIndex++;
    }
  }
  return {
    highBits: Long.fromBytes(highBytes),
    lowBits: Long.fromBytes(lowBytes),
  };
}
