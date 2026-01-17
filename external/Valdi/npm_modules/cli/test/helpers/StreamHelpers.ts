export function* eachLine(chunkOrString: Buffer | string): Generator<Buffer | string> {
  let position = 0;
  const chunk = typeof chunkOrString === 'string' ? Buffer.from(chunkOrString) : chunkOrString;
  while (position < chunk.length) {
    const newline = chunk.indexOf('\n', position);
    if (newline === -1) {
      yield chunk.subarray(position).toString('utf8') + '\n';
      break;
    }
    yield chunk.subarray(position, newline).toString('utf8') + `\n`;
    position = newline + 1;
  }
}

export async function* pipeEachLine(source: AsyncGenerator<Buffer | string>): AsyncGenerator<Buffer | string> {
  for await (const chunk of source) {
    for (const line of eachLine(chunk)) {
      yield line;
    }
  }
}

/**
 * Reads the input of source and calls `fn` for each line as a side-effect.
 *
 * The input/output is not changed and cannot be mutated by the callback.
 */
export function onEachLine(
  fn: (line: Buffer) => void,
): (source: AsyncGenerator<Buffer | string>) => AsyncGenerator<string | Buffer> {
  return async function* (source) {
    for await (const line of pipeEachLine(source)) {
      fn(typeof line === 'string' ? Buffer.from(line) : line);
      yield line;
    }
  };
}
