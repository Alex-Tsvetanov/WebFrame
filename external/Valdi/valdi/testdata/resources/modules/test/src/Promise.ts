

export async function getStringAsync(): Promise<string> {
  return 'Hello World Async';
}

export function getStringSync(): string {
  return 'Hello World Sync';
}

export async function concatStrings(left: Promise<string>, right: Promise<string>, onStep?: (step: number) => void): Promise<string> {
  if (onStep) {
    onStep(1);
  }
  const leftValue = await left;
  if (onStep) {
    onStep(2);
  }
  const rightValue = await right;
  if (onStep) {
    onStep(3);
  }

  return `${leftValue}_${rightValue}`;
}

export async function concatStringsComplex(prefix: Promise<Promise<string>>, array: Promise<string>[]): Promise<string> {
  const resolvedPrefix = await prefix;
  const suffixes = await Promise.all(array);

  return `${resolvedPrefix}->${suffixes.join('_')}`;
}
